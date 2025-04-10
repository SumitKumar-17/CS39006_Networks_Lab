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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define PORT 2525

// Response codes
#define OK "200 OK\n"
#define ERR "400 ERR\n"
#define NOT_FOUND "401 NOT FOUND\n"
#define BAD_SEQ_MSG "402 MAIL FROM AND RCPT TO NOT GIVEN\n"
#define FORBIDDEN "403 FORBIDDEN\n"
#define SERVER_ERROR "500 SERVER ERROR\n"

// Structure to hold client information
typedef struct {
    int socket;
    struct sockaddr_in address;
    int addr_len;
    int index;
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void create_mailbox_dir() {
    struct stat st = {0};
    //check if the foler does not exits then only create
    if (stat("mailbox", &st) == -1) {
        mkdir("mailbox", 0700);
    }
}

//getting the date-month-year
void get_current_date(char *date_str) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(date_str, 11, "%d-%m-%Y", tm_info);
}

//function which counts the total email received to a person
int count_emails(const char *recipient) {
    char filename[BUFFER_SIZE];
    sprintf(filename, "mailbox/%s.txt", recipient);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }
    
    int count = 0;
    char line[BUFFER_SIZE];
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "===EMAIL_START===", 17) == 0) {
            count++;
        }
    }
    
    fclose(file);
    return count;
}

//store the email details into the file
int store_email(const char *sender, const char *recipient, const char *body) {
    char filename[BUFFER_SIZE];
    sprintf(filename, "mailbox/%s.txt", recipient);
    
    FILE *file = fopen(filename, "a+");
    if (file == NULL) {
        perror("Error opening mailbox file");
        return -1;
    }
    
    char date[11];
    get_current_date(date);
    
    fprintf(file, "===EMAIL_START===\n");
    fprintf(file, "From: %s\n", sender);
    fprintf(file, "Date: %s\n", date);
    fprintf(file, "%s\n", body);
    fprintf(file, "===EMAIL_END===\n\n");
    
    fclose(file);
    return 0;
}

//list the email details for a person
char* list_emails(const char *recipient) {
    char filename[BUFFER_SIZE];
    sprintf(filename, "mailbox/%s.txt", recipient);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return strdup(NOT_FOUND);
    }
    
    static char list_buffer[BUFFER_SIZE * MAX_CLIENTS];
    strcpy(list_buffer, OK);
    
    char line[BUFFER_SIZE];
    int email_id = 0;
    char current_sender[BUFFER_SIZE] = "";
    char current_date[BUFFER_SIZE] = "";
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; 

        if (strncmp(line, "===EMAIL_START===", 17) == 0) {
            email_id++;
        } else if (strncmp(line, "From: ", 6) == 0) {
            strcpy(current_sender, line + 6);
        } else if (strncmp(line, "Date: ", 6) == 0) {
            strcpy(current_date, line + 6);
            char email_entry[BUFFER_SIZE];
            snprintf(email_entry, sizeof(email_entry), "%d: Email from %s (%s)\n", email_id, current_sender, current_date);
            strcat(list_buffer, email_entry);
        }
    }
    
    fclose(file);
    return list_buffer;
}

//function which all the emails details of a particular email id 
char* get_email(const char *recipient, int email_id) {
    char filename[BUFFER_SIZE];
    sprintf(filename, "mailbox/%s.txt", recipient);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return strdup(NOT_FOUND);
    }
    
    static char email_buffer[BUFFER_SIZE * MAX_CLIENTS];
    strcpy(email_buffer, OK);
    
    char line[BUFFER_SIZE];
    int current_id = 0;
    int in_target_email = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "===EMAIL_START===", 17) == 0) {
            current_id++;
            if (current_id == email_id) {
                in_target_email = 1;
            }
        } else if (strncmp(line, "===EMAIL_END===", 15) == 0) {
            if (in_target_email) {
                break;
            }
        } else if (in_target_email) {
            //storing all the contents of the email
            //between the EMAIL_START and the EMAIL_END keyword
            strcat(email_buffer, line);
        }
    }
    
    fclose(file);
    
    if (!in_target_email) {
        return strdup(NOT_FOUND);
    }
    
    return email_buffer;
}

//adding the client to the clients[]
//mutex to prevent any parallel changes
void add_client(client_t *client) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = client;
            clients[i]->index = i;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//removing the client when it got disconnected
void remove_client(int index) {
    pthread_mutex_lock(&clients_mutex);
    if (clients[index]) {
        close(clients[index]->socket);
        free(clients[index]);
        clients[index] = NULL; //assigning it to null so that 
        //a reused index can be used again
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    //take the void *arg and convert to a client_t *arg type
    client_t *client = (client_t *)arg;

    char buffer[BUFFER_SIZE];
    char sender[BUFFER_SIZE] = "";
    char recipient[BUFFER_SIZE] = "";
    char client_id[BUFFER_SIZE] = "";
    int session_active = 1;
    
    printf("Client connected: %s\n", inet_ntoa(client->address.sin_addr));
    
    while (session_active) {
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = recv(client->socket, buffer, BUFFER_SIZE, 0);
        
        if (read_size <= 0) {
            printf("Client disconnected.\n");
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = 0; 
        printf("Received: %s\n", buffer);
       
        //for the HELO command
        if (strncmp(buffer, "HELO ", 5) == 0) {
            strcpy(client_id, buffer + 5);
            printf("HELO received from %s\n", client_id);
            send(client->socket, OK, strlen(OK), 0);
        } 
        //for the MAIL FROM: command. Read the mail and then send the status response to the client back
        else if (strncmp(buffer, "MAIL FROM: ", 11) == 0) {
            strcpy(sender, buffer + 11);
            printf("MAIL FROM: %s\n", sender);
            send(client->socket, OK, strlen(OK), 0);
        } 
        //for the RCPT TO commnad
        else if (strncmp(buffer, "RCPT TO: ", 9) == 0) {
            strcpy(recipient, buffer + 9);
            printf("RCPT TO: %s\n", recipient);
            send(client->socket, OK, strlen(OK), 0);
        } 
        //for the DATA command
        else if (strcmp(buffer, "DATA") == 0) {
            if (strlen(sender) == 0 || strlen(recipient) == 0) {
                send(client->socket, BAD_SEQ_MSG, strlen(BAD_SEQ_MSG), 0);
                continue;
            }
            
            send(client->socket, "Enter your message (end with a single dot '.'):\n", 47, 0);
            
            char message[BUFFER_SIZE * MAX_CLIENTS] = "";
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                read_size = recv(client->socket, buffer, BUFFER_SIZE, 0);
                
                if (read_size <= 0) {
                    session_active = 0;
                    break;
                }
                
                buffer[strcspn(buffer, "\n")] = 0; 
                
                if (strcmp(buffer, ".") == 0) {
                    break;
                }
                
                strcat(message, buffer);
                strcat(message, "\n");
            }
            
            printf("DATA received, message stored.\n");
            
            if (store_email(sender, recipient, message) == 0) {
                send(client->socket, "200 Message stored successfully\n", 32, 0);
            } else {
                send(client->socket, SERVER_ERROR, strlen(SERVER_ERROR), 0);
            }
        } 
        //for the LIST commands
        // the list_emails() fetches all the details in short
        else if (strncmp(buffer, "LIST ", 5) == 0) {
            char list_recipient[BUFFER_SIZE];
            strcpy(list_recipient, buffer + 5);
            printf("LIST %s\n", list_recipient);
            
            char *list = list_emails(list_recipient);
            send(client->socket, list, strlen(list), 0);
            printf("Emails retrieved; list sent.\n");
            if (strncmp(list, OK, strlen(OK)) != 0) {
                free(list);
            }
        } 
        //for the GET_MAIL 
        //uses the get_mail and fetches all the details of a particular email.
        else if (strncmp(buffer, "GET_MAIL ", 9) == 0) {
            char get_recipient[BUFFER_SIZE];
            int get_id;
            sscanf(buffer + 9, "%s %d", get_recipient, &get_id);
            printf("GET_MAIL %s %d\n", get_recipient, get_id);
            
            char *email = get_email(get_recipient, get_id);
            send(client->socket, email, strlen(email), 0);
            printf("Email with id %d sent.\n", get_id);
            if (strncmp(email, OK, strlen(OK)) != 0 && strncmp(email, NOT_FOUND, strlen(NOT_FOUND)) != 0) {   
                free(email);
            }
        } 
        //Quit and send goodbye to client
        else if (strcmp(buffer, "QUIT") == 0) {
            send(client->socket, "200 Goodbye\n", 12, 0);
            session_active = 0;
            printf("Client requested to quit.\n");
        } 
        //Random errors
        else {
            send(client->socket, ERR, strlen(ERR), 0);
        }
    }
    
    remove_client(client->index);
    printf("Client disconnected.\n");
    
    return NULL;
}

int main(int argc, char *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t thread_id;
    
    //a function which checks if the mailbox directory is there or not
    //if not create it
    create_mailbox_dir();
    
    // assign the port from commnad line 
    //if port not given then start the server at default port 2525
    int port = PORT;
    if (argc >= 2) {
        port = atoi(argv[1]);
    }
    
    //create a tcp socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(1);
    }
     
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(1);
    }
    
    //setting the server variables
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    //bind the server_fd to the server address and the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // making a listen to listen for amx numebr of connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("Listening on port %d...\n", port);
    
    //assign the total clients to NULL
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = NULL;
    }
    
    while (1) {
        //wait to be connected by a client
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        //when you get a connection then assign that address and the sokcet_fd to the client[i]
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->socket = new_socket;
        client->address = address;
        client->addr_len = addrlen;
        
        add_client(client);
        // create a thread for each of the client d=so that they can access
        // the same file and mutex locks being shared in the program
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client) != 0) {
            perror("Thread creation failed");
            close(new_socket);
            free(client);
            continue;
        }
        
        pthread_detach(thread_id);
    }
    
    return 0;
}