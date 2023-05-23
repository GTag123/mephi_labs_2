#include "tcp_connect.h"
#include "byte_tools.h"

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

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout):
        port_(port), ip_(ip), connectTimeout_(connectTimeout), readTimeout_(readTimeout) {}

TcpConnect::~TcpConnect() {
    if (sock_ >= 0) close(sock_);
}

void TcpConnect::ChangeSocketBlocking() const {
    int currentFlags = fcntl(sock_, F_GETFL, 0);
    if (currentFlags==-1) throw std::runtime_error("error while changing socket blocking");

    if ((currentFlags&O_NONBLOCK) == 0) {currentFlags |= O_NONBLOCK;}
    else                                {currentFlags &= ~O_NONBLOCK;}

    if (fcntl(sock_, F_SETFL, currentFlags)<0) throw std::runtime_error("error while setting flags");
}

void TcpConnect::EstablishConnection() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_<0) throw std::runtime_error("bad socket creation");

    struct sockaddr_in connectData;
    connectData.sin_family = AF_INET;
    connectData.sin_port = htons(port_);
    if (inet_pton(AF_INET, ip_.data(), &connectData.sin_addr)<= 0) {throw std::runtime_error("error while converting ip-address");}

    ChangeSocketBlocking();

    fd_set fdset;
    struct timeval tv;
    FD_ZERO(&fdset);
    FD_SET(sock_, &fdset);
    tv.tv_sec = connectTimeout_.count()/1000;
    tv.tv_usec = (connectTimeout_.count()%1000)*1000;

    connect(sock_, (struct sockaddr*) &connectData, sizeof(connectData));
    int select_res = select(sock_+1, NULL, &fdset, NULL, &tv);
    if (select_res!=1) throw std::runtime_error("connecting TimeOut");
    int so_error;
    socklen_t len = sizeof so_error;
    getsockopt(sock_, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error!=0) throw std::runtime_error("error while connecting: " + std::string(strerror(so_error)));
    //connected
    ChangeSocketBlocking();
    return;
}

void TcpConnect::SendData(const std::string& data) const {
    if (send(sock_, data.c_str(), data.size(), 0)<0) {
        throw std::runtime_error("error while sending");
    }
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    if (bufferSize==0) {
        ChangeSocketBlocking();

        struct pollfd fd;
        fd.fd = sock_;
        fd.events = POLLIN;
        int pres = poll(&fd, 1, readTimeout_.count()/2);

        if (pres < 0) throw std::runtime_error("poll");
        if (pres == 0) throw std::runtime_error("reading first 4 bytes TimeOut");

        unsigned char small_buffer[4];
        ChangeSocketBlocking();
        if (recv(sock_, small_buffer, 4, 0)<4) throw std::runtime_error("read <4 bytes");
        bufferSize = BytesToInt(std::string(reinterpret_cast<char*>(small_buffer), 4));
    }
    size_t remained = bufferSize;
    //std::cout << "remained bytes = " << remained << std::endl;
    if (bufferSize>=1<<16) {bufferSize = 1<<16;}
    unsigned char buffer[bufferSize];
    std::string ans;
    std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
    while (remained>0) {
        std::chrono::duration<double, std::milli> diff = std::chrono::steady_clock::now()-startTime;
        if (diff.count() > readTimeout_.count()/2) throw std::runtime_error("reading TimeOut");

        int received = recv(sock_, buffer, std::min(remained, bufferSize), 0);
        //if (received!=0) std::cout << "received " << received << " bytes" << std::endl;
        if (received<0) throw std::runtime_error("error while reading data");
        remained-=received;
        for (int i=0; i<received; i++) {
            ans+=buffer[i];
        }
    }
    return ans;
}

void TcpConnect::CloseConnection() {
    if (sock_ >= 0) close(sock_);
}

const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}