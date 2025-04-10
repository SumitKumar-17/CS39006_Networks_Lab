/*
Name Sumit Kumar
Roll NO: 22CS30056
LT_2
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

#define PORT 9090
#define BUFFER_SIZE 4096

void set_udp_socket_options(int sockfd);
void log_message(const char *client_ip, int client_port, const char *msg);
int create_udp_server_socket(int port);

int main() {
    int sockfd = create_udp_server_socket(PORT);
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    struct timeval timeout;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity > 0) {
            if (FD_ISSET(sockfd, &readfds)) {
                ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
                if (len > 0) {
                    buffer[len] = '\0';
                    log_message(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
                    sendto(sockfd, buffer, len, 0, (struct sockaddr *)&client_addr, addr_len);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}

int create_udp_server_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    set_udp_socket_options(sockfd);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }

    return sockfd;
}

void set_udp_socket_options(int sockfd) {
    int buffer_size = BUFFER_SIZE;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        exit(1);
    }

    int current_size;
    socklen_t optlen = sizeof(current_size);
    if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &current_size, &optlen) < 0) {
        perror("getsockopt failed");
        close(sockfd);
        exit(1);
    }

    printf("Receive buffer size: %d bytes\n", current_size);

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
}

void log_message(const char *client_ip, int client_port, const char *msg) {
    FILE *logfile = fopen("udp_server.log", "a");
    if (logfile == NULL) {
        perror("Log file open failed");
        exit(1);
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(logfile, "[%s] From %s:%d - %s\n", time_str, client_ip, client_port, msg);
    fclose(logfile);

    printf("[%s] From %s:%d - %s\n", time_str, client_ip, client_port, msg);
}
