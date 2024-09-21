#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA ws;
    int status = 0;

    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *res, *itr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, "2020", &hints, &res);
    if (status != 0) {
        printf("getaddrinfo failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    SOCKET serverSocket = INVALID_SOCKET;
    for (itr = res; itr != NULL; itr = itr->ai_next) {
        serverSocket = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
        if (serverSocket == INVALID_SOCKET) {
            continue;
        }

        if (itr->ai_family == AF_INET6) {
            int off = 0;
            if (setsockopt(serverSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off)) == SOCKET_ERROR) {
                printf("Failed to set IPV6_V6ONLY: ON. Error: %d\n", WSAGetLastError());
                closesocket(serverSocket);
                serverSocket = INVALID_SOCKET;
                continue;
            }
        }

        if (bind(serverSocket, itr->ai_addr, (int)itr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(serverSocket);
            serverSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(res);

    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation or bind failure with error: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    status = listen(serverSocket, 5);
    if (status == SOCKET_ERROR) {
        printf("Listen failure with error: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 2020 (IPv4 and IPv6)\n");

    struct sockaddr_storage their_info;
    socklen_t socksize = sizeof(their_info);
    SOCKET maxfd;

    fd_set fr, master;
    FD_ZERO(&fr);
    FD_ZERO(&master);

    FD_SET(serverSocket, &master);
    maxfd = serverSocket;

    while (1) {
        fr = master;
        status = select(maxfd + 1, &fr, NULL, NULL, NULL);
        if (status == SOCKET_ERROR) {
            printf("Select failure with error: %d\n", WSAGetLastError());
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        for (int d = 0; d <= maxfd; d++) {
            if (FD_ISSET(d, &fr)) {
                if (d == serverSocket) {
                    SOCKET newfd = accept(serverSocket, (struct sockaddr*)&their_info, &socksize);
                    if (newfd == INVALID_SOCKET) {
                        printf("Accept failure with error: %d\n", WSAGetLastError());
                        continue;
                    }

                    printf("New client connected on socket %d\n", newfd);
                    
                    FD_SET(newfd, &master);
                    if (newfd > maxfd) maxfd = newfd;

                }
                else {
                    char buffer[1024];
                    int bufferlen = sizeof(buffer);

                    int bytes_received = recv(d, buffer, bufferlen, 0);
                    if (bytes_received <= 0) {
                        if (bytes_received == 0) {
                            printf("Client on socket %d disconnected\n", d);
                        }
                        else {
                            printf("Recv failure on socket %d with error: %d\n", d, WSAGetLastError());
                        }
                        closesocket(d);
                        FD_CLR(d, &master);
                        continue;
                    }

                    // Broadcast to all other clients
                    char header[1024];
                    char socket_str[10];
                    _snprintf_s(socket_str, sizeof(socket_str), _TRUNCATE, "%d", d);
                    _snprintf_s(header, sizeof(header), _TRUNCATE, "Socket %s: %.*s", socket_str, bytes_received, buffer);

                    for (int e = 0; e <= maxfd; e++) {
                        if (FD_ISSET(e, &master)) {
                            if (e != serverSocket && e != d) {
                                if (send(e, header, (int)strlen(header), 0) == SOCKET_ERROR) {
                                    printf("Send failure on socket %d with error: %d\n", e, WSAGetLastError());
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}