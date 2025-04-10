/*
=============================================================================================================
    Assignment 2 Submission
    Name: Sumit Kumar
    Roll number: 22CS30056
    Link of the pcap file:
https://drive.google.com/file/d/17iV9UOhMwdw-s0eDuar8RAsC6azpP0X6/view?usp=sharing
=============================================================================================================
*/

// This is the server code for the wordserver which sends the words from the
// file to the client
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 1024

// Sending message to the client by the server
void send_message(int sockfd, struct sockaddr_in *client_addr,
                  socklen_t client_len, const char *message) {
  if (sendto(sockfd, message, strlen(message) + 1, 0,
             (struct sockaddr *)client_addr, client_len) == -1) {
    perror("Failed to send message");
    exit(EXIT_FAILURE);
  }
}

int main() {
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE], filename[BUFFER_SIZE], word[BUFFER_SIZE];
  FILE *file;

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  // Create a UDP Socket
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // binding the servere address to the socket descriptor
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // starting the connection if everything is fine
  printf("Server listening on port %d...\n", PORT);

  // an infinite loop to keep the server running
  while (1) {
    // Receiving the filename from the client
    int len = recvfrom(sockfd, filename, BUFFER_SIZE - 1, 0,
                       (struct sockaddr *)&client_addr, &client_len);
    filename[len] = '\0';
    printf("Received file request: %s\n", filename);

    // Open the file in the current directory only
    file = fopen(filename, "r");
    if (!file) {
      snprintf(buffer, BUFFER_SIZE, "NOTFOUND %s", filename);
      send_message(sockfd, &client_addr, client_len, buffer);
      continue;
    }

    // Check for "HELLO" if the file is not empty
    if (!fgets(word, BUFFER_SIZE, file)) {
      fclose(file);
      snprintf(buffer, BUFFER_SIZE, "ERROR: File %s is empty or unreadable",
               filename);
      send_message(sockfd, &client_addr, client_len, buffer);
      continue;
    }

    word[strcspn(word, "\n")] = '\0';
    // If the first word is not "HELLO", send an error message and the
    // transmission is stopped
    if (strcmp(word, "HELLO") != 0) {
      fclose(file);
      snprintf(buffer, BUFFER_SIZE, "ERROR: File %s does not start with HELLO",
               filename);
      send_message(sockfd, &client_addr, client_len, buffer);
      continue;
    }

    // send the first word to the client
    send_message(sockfd, &client_addr, client_len, word);

    // Send the rest of the words in the file
    while (fgets(word, BUFFER_SIZE, file)) {
      word[strcspn(word, "\n")] = '\0';
      send_message(sockfd, &client_addr, client_len, word);
      printf("Sent: %s\n", word);
      if (strcmp(word, "FINISH") == 0) {
        // break;
        fclose(file);
        close(sockfd);
        printf("FINISH received. Server shutting down.\n");
        return 0; // Exit the server after FINISH is received
      }
    }

    fclose(file);
  }

  close(sockfd);
  return 0;
}
