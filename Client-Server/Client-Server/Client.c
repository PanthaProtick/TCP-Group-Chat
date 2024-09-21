#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include<Windows.h>

DWORD WINAPI recieve_messages(LPVOID socket) {
    SOCKET clientSocket = *(SOCKET*)socket;
    char buffer[1024];
    int bytes_recieved;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        bytes_recieved = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytes_recieved == 0) {
            printf("Disconnected from the server\n");
            break;
        }
        else if (bytes_recieved < 0) {
            printf("Recv failure. Code: %d\n", WSAGetLastError());
            break;
        }
        else {
            printf("\n%s\n\n",buffer);
        }
    }

    return 0;
}

int main()
{
    WSADATA wsaData;
    int status;
    char ip[] = "127.0.0.1";
    char port[] = "2020";

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    struct addrinfo hints, * res, * itr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo(ip, port, &hints, &res);
    if (status != 0) {
        printf("getaddrinfo failed with error: %d\n", status);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    SOCKET clientSocket = INVALID_SOCKET;
    for (itr = res; itr != NULL; itr = itr->ai_next) {
        clientSocket = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
        status = connect(clientSocket, itr->ai_addr, itr->ai_addrlen);
        if (status == SOCKET_ERROR) {
            closesocket(clientSocket);
            continue;
        }
        break;
    }
    
    freeaddrinfo(res);
    
    if (itr==NULL) {
        printf("Connection error %d\n",WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server. Enter your message or type 'exit' to close connection.\n");

    HANDLE message_reciever = CreateThread(NULL, 0, recieve_messages, (void*)&clientSocket, 0, NULL);

    char msg[1024];
    int bytes_sent;

    while (1) {
        memset(msg, 0, sizeof(msg));
        fgets(msg, sizeof(msg), stdin);
        //msg[strcspn(msg, "\n")] = '\0';
        printf("\n");
        if (!strcmp(msg, "exit\n")) break;
        bytes_sent = send(clientSocket, msg, strlen(msg), 0);
        if (bytes_sent < 0) {
            printf("Send failure. Code: %d\n", WSAGetLastError());
        }
    }

    shutdown(clientSocket, SD_BOTH);

    WaitForSingleObject(message_reciever, INFINITE);

    closesocket(clientSocket);
    CloseHandle(message_reciever);
    WSACleanup();
    return 0;
}