/*
=====================================
Assignment 5 Submission
Name: Sumit Kumar
Roll number: 22CS30056
=====================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define MAX_RETRIES 10

int perform_server_task(const char* task) {
    int a, b;
    char op;

    if (sscanf(task, "%d %c %d", &a, &op, &b) != 3) {
        fprintf(stderr, "Invalid task format: %s\n", task);
        return 0;
    }

    switch (op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            if (b == 0) {
                fprintf(stderr, "Division by zero!\n");
                return 0;
            }
            return a / b;
        default:
            fprintf(stderr, "Unsupported operation: %c\n", op);
            return 0;
    }
}

// Signal handler for SIGINT (Ctrl+C)
// To close the client 
volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    (void)sig;  
    keep_running = 0;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    signal(SIGINT, handle_sigint);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 addr to binary format
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return -1;
    }

    // During the first connection set the BLOCKING MODE
    printf("Connecting to server at %s:%d...\n", SERVER_IP, SERVER_PORT);
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Now make the socket to NON-BLOCKING MODE
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        return -1;
    }

    while (keep_running) {
        // GET_TASK request sending
        printf("Requesting task from server...\n");
        const char* request = "GET_TASK";
        if (send(sock, request, strlen(request), 0) < 0) {
            perror("Send failed");
            break;
        }

        // Wait for response polling after retries
        memset(buffer, 0, sizeof(buffer));
        int received = 0;
        int retry_count = 0;

        printf("Waiting for server response...\n");

        while (!received && retry_count < MAX_RETRIES && keep_running) {
            int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);

            if (bytes > 0) {
                buffer[bytes] = '\0';
                received = 1;
                printf("Received %d bytes from server\n", bytes);
            } else if (bytes == 0) {
                printf("Server disconnected\n");
                close(sock);
                return 0;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv failed");
                close(sock);
                return -1;
            }

            if (!received) {
                printf("No data yet, retrying (%d/%d)...\n", retry_count + 1, MAX_RETRIES);
                sleep(1);  
                retry_count++;
            }
        }

        if (!received) {
            fprintf(stderr, "Timeout waiting for server response\n");
            close(sock);
            return -1;
        }

        printf("Server response: %s", buffer);
        if (strncmp(buffer, "Task:", 5) == 0) {
            char task[BUFFER_SIZE];
            if (sscanf(buffer, "Task: %[^\n]", task) != 1) {
                printf("Error parsing task: %s\n", buffer);
                sleep(2);
                continue;
            }
            
            printf("Processing task: '%s'\n", task);

            int result = perform_server_task(task);
            printf("Calculated result: %d\n", result);

            // Send result back to server
            char result_msg[BUFFER_SIZE];
            snprintf(result_msg, BUFFER_SIZE, "RESULT %d", result);
            if (send(sock, result_msg, strlen(result_msg), 0) < 0) {
                perror("Send failed");
                break;
            }

            // Wait for ACK
            memset(buffer, 0, BUFFER_SIZE);
            received = 0;
            retry_count = 0;

            while (!received && retry_count < MAX_RETRIES && keep_running) {
                int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    received = 1;
                    printf("Received %d bytes from server\n", bytes);
                } 
                else if (bytes == 0) {
                    printf("Server disconnected\n");
                    close(sock);
                    return 0;
                }
                else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recv failed");
                    close(sock);
                    return -1;
                }
                
                if (!received) {
                    printf("No data yet, retrying (%d/%d)...\n", retry_count + 1, MAX_RETRIES);
                    sleep(1);  
                    retry_count++;
                }
            }

            if (received) {
                printf("Server acknowledgment: %s", buffer);
            } else {
                fprintf(stderr, "Timeout waiting for server acknowledgment\n");
                break;
            }

            //NOT that is thei implemenetation at a time a client can be given only one process
            // So even if someonce makes threads or process to call multiple task by a single clinet only  this does nothappens
            sleep(1 + rand() % 3);
        } else if (strstr(buffer, "No tasks available") != NULL) {
            printf("No more tasks available. Exiting.\n");
            
            const char* exit_msg = "exit";
            send(sock, exit_msg, strlen(exit_msg), 0);
            
            memset(buffer, 0, BUFFER_SIZE);
            received = 0;
            retry_count = 0;
            
            while (!received && retry_count < MAX_RETRIES && keep_running) {
                int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    received = 1;
                    printf("Server goodbye: %s", buffer);
                } 
                else if (bytes == 0) {
                    printf("Server disconnected\n");
                    break;
                }
                else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recv failed");
                    break;
                }
                
                if (!received) {
                    printf("Waiting for server goodbye (%d/%d)...\n", retry_count + 1, MAX_RETRIES);
                    sleep(1);
                    retry_count++;
                }
            }
            
            break;
        } else if (strstr(buffer, "Error") != NULL) {
            printf("Server reported an error: %s", buffer);
            sleep(2);
        } else {
            printf("Unexpected response from server: %s", buffer);
            sleep(2);
        }
    }

    close(sock);
    return 0;
}