#pragma once

#include <string>
#include <chrono>
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
/*
 * Обертка над низкоуровневой структурой сокета.
 */
class TcpConnect {
public:
    TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout) :
            ip_(std::move(ip)), port_(port), connectTimeout_(connectTimeout), readTimeout_(readTimeout) {}
    ~TcpConnect() {
        if (status != 2){
            CloseConnection();
        }
    };

    /*
     * Установить tcp соединение.
     * Если соединение занимает более `connectTimeout` времени, то прервать подключение и выбросить исключение.
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man7/socket.7.html
     * - https://man7.org/linux/man-pages/man2/connect.2.html
     * - https://man7.org/linux/man-pages/man2/fcntl.2.html (чтобы включить неблокирующий режим работы операций)
     * - https://man7.org/linux/man-pages/man2/select.2.html
     * - https://man7.org/linux/man-pages/man2/setsockopt.2.html
     * - https://man7.org/linux/man-pages/man2/close.2.html
     * - https://man7.org/linux/man-pages/man3/errno.3.html
     * - https://man7.org/linux/man-pages/man3/strerror.3.html
     */
    void EstablishConnection(){
        struct pollfd pfd;
        int ret;

        // Set up the socket address and port
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(ip_.c_str());
        address.sin_port = htons(port_);
        // Create the socket
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Set socket to non-blocking mode
        int flags = fcntl(sockfd_, F_GETFL, 0);
        fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);

        // Connect to the remote host
        ret = connect(sockfd_, (struct sockaddr*)&address, sizeof(address));
        if (ret == 0) {
            // Connection established immediately
            flags = fcntl(sockfd_, F_GETFL, 0);
            fcntl(sockfd_, F_SETFL, flags & ~O_NONBLOCK);
            return;
        }

        // Wait for connection to be established, or for timeout to occur
        pfd.fd = sockfd_;
        pfd.events = POLLOUT;
        ret = poll(&pfd, 1, connectTimeout_.count());
        if (ret == 0) {
            // Timeout occurred
            throw std::runtime_error("Connection timed out");
        } else if (ret < 0) {
            // Error occurred
            throw std::runtime_error("Error while connecting to remote host");
        } else {
            // Connection established
            flags = fcntl(sockfd_, F_GETFL, 0);
            fcntl(sockfd_, F_SETFL, flags & ~O_NONBLOCK);
            return;
        }
    };

    /*
     * Послать данные в сокет
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man2/send.2.html
     */
    void SendData(const std::string& data) const{
        const char* buf = data.c_str();
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

    /*
     * Прочитать данные из сокета.
     * Если передан `bufferSize`, то прочитать `bufferSize` байт.
     * Если параметр `bufferSize` не передан, то сначала прочитать 4 байта, а затем прочитать количество байт, равное
     * прочитанному значению.
     * Первые 4 байта (в которых хранится длина сообщения) интерпретируются как целое число в формате big endian,
     * см https://wiki.theory.org/BitTorrentSpecification#Data_Types
     * Полезная информация:
     * - https://man7.org/linux/man-pages/man2/poll.2.html
     * - https://man7.org/linux/man-pages/man2/recv.2.html
     */
    std::string ReceiveData(size_t bufferSize = 0) const{
        int len;
        if (bufferSize > 0) {
            len = bufferSize;
        } else {
            // read message length
            char lenbuf[4];
            struct timeval tv;
            tv.tv_sec = readTimeout_.count() / 1000;
            tv.tv_usec = (readTimeout_.count() % 1000) * 1000;
            if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
                throw std::runtime_error("Failed to set socket timeout");
            }
            int n = recv(sockfd_, lenbuf, 4, 0);
            if (n < 0) {
                throw std::runtime_error("Failed to receive data");
            }
            if (n == 0) {
                return "";
            }
            if (n < 4) {
                throw std::runtime_error("Invalid message length");
            }
            len = BytesToInt(lenbuf);
        }

        // read message
        std::string data(len, 0);
        char* buf = &data[0];
        while (len > 0) {
            struct timeval tv;
            tv.tv_sec = readTimeout_.count() / 1000;
            tv.tv_usec = (readTimeout_.count() % 1000) * 1000;
            if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
                throw std::runtime_error("Failed to set socket timeout");
            }
            int n = recv(sockfd_, buf, len, 0);
            if (n < 0) {
                throw std::runtime_error("Failed to receive data");
            }
            if (n == 0) {
                return "";
            }
            buf += n;
            len -= n;
        }
        return data;
    }

    /*
     * Закрыть сокет
     */
    void CloseConnection() {
        close(sockfd_);
        status = 2;
    }

    const std::string& GetIp() const {
        return ip_;
    }
    int GetPort() const {
        return port_;
    }
private:
    const std::string ip_;
    const int port_;
    std::chrono::milliseconds connectTimeout_, readTimeout_;
    int sockfd_;
    int status = 0; // 0 - не открыто, 1 - открыто, 2 - закрыто
};


