#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <limits>
#include <utility>
#include "tcp_connect.h"

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout,
                       std::chrono::milliseconds readTimeout) :
        ip_(std::move(ip)), port_(port), connectTimeout_(connectTimeout), readTimeout_(readTimeout) {}

TcpConnect::~TcpConnect() {
    if (status != 2) {
        CloseConnection();
    }
}

void TcpConnect::EstablishConnection() {
    struct pollfd pfd;
    int ret;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip_.c_str());
    address.sin_port = htons(port_);
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    int flags = fcntl(sockfd_, F_GETFL, 0);
    fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);
    ret = connect(sockfd_, (struct sockaddr *) &address, sizeof(address));
    if (ret == 0) {
        // immediately
        flags = fcntl(sockfd_, F_GETFL, 0);
        fcntl(sockfd_, F_SETFL, flags & ~O_NONBLOCK);
        return;
    }
    pfd.fd = sockfd_;
    pfd.events = POLLOUT;
    ret = poll(&pfd, 1, connectTimeout_.count());
    if (ret == 0) {
        throw std::runtime_error("Connection timed out");
    } else if (ret < 0) {
        throw std::runtime_error("Error while connecting to remote host");
    } else {
        flags = fcntl(sockfd_, F_GETFL, 0);
        fcntl(sockfd_, F_SETFL, flags & ~O_NONBLOCK);
        return;
    }
}

void TcpConnect::SendData(const std::string &data) const {
    const char *buf = data.c_str();
    int len = data.length();
    while (len > 0) {
        int sent = send(sockfd_, buf, len, 0);
        if (sent < 0) {
            throw std::runtime_error("Failed to send data");
        }
        buf += sent;
        len -= sent;
    }
}


std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    int len;
    if (bufferSize > 0) {
        len = bufferSize;
    } else {
        char lenbuf[4];
        struct timeval tv;
        tv.tv_sec = readTimeout_.count() / 1000;
        tv.tv_usec = (readTimeout_.count() % 1000) * 1000;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)) < 0) {
            throw std::runtime_error("Failed to set socket timeout");
        }
        int n = recv(sockfd_, lenbuf, 4, 0);
        if (n < 0) {
            throw std::runtime_error("Failed to receive data");
        }
//        if (n == 0) {
//            return "";
//        }
        if (n < 4) {
            throw std::runtime_error("Invalid message length");
        }
        len = BytesToInt(lenbuf);
    }
    std::string data(len, 0);
    char *buf = &data[0];
    while (len > 0) {
        struct timeval tv;
        tv.tv_sec = readTimeout_.count() / 1000;
        tv.tv_usec = (readTimeout_.count() % 1000) * 1000;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)) < 0) {
            throw std::runtime_error("Failed to set socket timeout");
        }
        int n = recv(sockfd_, buf, len, 0);
        if (n < 0) {
            throw std::runtime_error("Failed to receive data");
        }
//        if (n == 0) {
//            return "";
//        }
        buf += n;
        len -= n;
    }
    return data;
}

void TcpConnect::CloseConnection() {
    close(sockfd_);
    status = 2;
}

const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}