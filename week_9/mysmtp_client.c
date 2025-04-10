/*
=====================================
Assignment 6 Submission
Name: Sumit Kumar
Roll number: 22CS30056
=====================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

void display_available_commands() {
    printf("\n========================= My_SMTP Client Command Reference =========================\n");
    printf("HELO <client_id>         : Initiate session with the server (Example=> HELO example.com)\n");
    printf("MAIL FROM: <email>       : Specify sender's email address (Example=> MAIL FROM: sumit@gmail.com)\n");
    printf("RCPT TO: <email>         : Specify recipient's email address (Example=> RCPT TO: someone@gmail.com)\n");
    printf("DATA                     : Begin composing email message\n");
    printf("LIST <email>             : List all emails for recipient (Example=> LIST sumit@example.com)\n");
    printf("GET_MAIL <email> <id>    : Retrieve a specific email (Example=> GET_MAIL sumit@example.com 1)\n");
    printf("QUIT                     : End the session\n");
    printf("NOTE: In MAIL FROM and RCPT TO there is \'space\' after the \':\' symbol\n");
    printf("======================================================================================\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    
    //make a socketfd
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    //set the server_addr varibles
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    //convert the IP address in proper format
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(1);
    }
    
    //wait till you get a connection accepted by the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    
    printf("Connected to My_SMTP server.\n");

    //printing some helping commnads
    display_available_commands();
    while (1) {
        memset(input, 0, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE);
        
        printf("> ");
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        //quit the server by sending the quit message to the server
        if (strcmp(input, "QUIT") == 0) {
            send(sockfd, input, strlen(input), 0);
            
            int bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (bytes_recv <= 0) break;
            
            buffer[bytes_recv] = '\0';
            printf("%s", buffer);
            break;
        }
        
        // entering og the data by the user
        if (strcmp(input, "DATA") == 0) {
            send(sockfd, input, strlen(input), 0);
            
            int bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (bytes_recv <= 0) break;
            
            buffer[bytes_recv] = '\0';
            printf("%s", buffer);
            
            printf("Enter your message (end with a single dot '.'):\n");
            while (1) {
                memset(input, 0, BUFFER_SIZE);
                if (fgets(input, BUFFER_SIZE, stdin) == NULL) break;
                
                input[strcspn(input, "\n")] = 0;            
                send(sockfd, input, strlen(input), 0);
                if (strcmp(input, ".") == 0) break;
            }
            
            bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (bytes_recv <= 0) break;
            
            buffer[bytes_recv] = '\0';
            printf("%s", buffer);
            continue;
        }
        
        //in case of any other command than DATA send to the server.
        send(sockfd, input, strlen(input), 0);
        
        //client is waiting for any type of acknowledgement message from the server
        int bytes_recv = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_recv <= 0) {
            printf("Server disconnected.\n");
            break;
        }
        
        buffer[bytes_recv] = '\0';
        printf("%s", buffer);
    }
    
    //close the socket
    close(sockfd);
    return 0;
}