/*
Name Sumit Kumar
Roll NO: 22CS30056
LT_2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 9090
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // char message[] = "hello server";
    char message[1000];
    scanf("%s",message);
    if (sendto(sockfd, message, sizeof(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Send failed");
        close(sockfd);
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);
    ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &addr_len);
    if (len > 0) {
        buffer[len] = '\0';
        printf("Received: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
