#include <errno.h>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <cstring>
#include "TreadPool.h"
constexpr int kPort = 8081;
constexpr int kBufSize = 1024;


class TSocketGuard {
    char buf[kBufSize];
    ThreadPool pool_;
public:
    TSocketGuard(int port, int workers): pool_(workers) {
        const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            perror("socket");
            exit(1);
        }
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(kPort);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("bind");
            exit(1);
        }

        if (listen(socket_fd, workers) < 0) {
            perror("listen");
            exit(1);
        }
        ServeForever(socket_fd);
        close(socket_fd);
    }
    ~TSocketGuard() {
        pool_.Terminate(true);
    };
    void Log(const sockaddr_in& addr) {
        std::string host(NI_MAXHOST, '\0');
        std::string port(NI_MAXSERV, '\0');

        const int err = getnameinfo(
                (struct sockaddr*)&addr, sizeof(addr), host.data(), host.size(), port.data(), port.size(), NI_NUMERICSERV);

        if (err < 0) {
            perror("getnameinfo");
            return;
        }

        std::cout << "Client: " << host.data() << ' ' << port.data() << std::endl;
    }

    void EchoHandler(int client_socket) {
        int n = 0;
        while ((n = recv(client_socket, buf, kBufSize, 0)) > 0) {
            std::cout << "Received: " << std::string(buf, kBufSize) << std::endl;
            if (send(client_socket, buf, n, 0) < 0) {
                perror("send");
                return;
            }
        }

        if (n < 0) {
            perror("recv");
        }
    }

    void ServeForever(int server_socket) {
        std::cout << "Server is running!!!" << std::endl;

        for (;;) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);

            const int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);

            if (client_socket < 0) {
                perror("accept");
                continue;
            }

            Log(client_addr);
            pool_.PushTask([client_socket, this] {
                EchoHandler(client_socket);
                close(client_socket);
            });
        }
    }
};




int main() {


    return 0;
}