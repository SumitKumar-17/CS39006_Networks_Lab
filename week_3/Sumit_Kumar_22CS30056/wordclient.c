/*
=============================================================================================================
    Assignment 2 Submission
    Name: Sumit Kumar
    Roll number: 22CS30056
    Link of the pcap file:
https://drive.google.com/file/d/17iV9UOhMwdw-s0eDuar8RAsC6azpP0X6/view?usp=sharing
=============================================================================================================
*/

// This is the client code for the wordclient which requests the words from the
// server and writes them to a file
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12345
#define BUF_SIZE 1024

// Sending message to the server by the client
void send_message(int sockfd, struct sockaddr_in *server_addr,
                  socklen_t server_len, const char *message) {
  if (sendto(sockfd, message, strlen(message) + 1, 0,
             (struct sockaddr *)server_addr, server_len) == -1) {
    perror("Failed to send message");
    exit(EXIT_FAILURE);
  }
}

// Receiving message from the server by the client
int receive_message(int sockfd, struct sockaddr_in *server_addr,
                    socklen_t *server_len, char *buffer) {
  int len = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                     (struct sockaddr *)server_addr, server_len);
  if (len < 0) {
    perror("Failed to receive message");
    exit(EXIT_FAILURE);
  }
  buffer[len] = '\0';
  return len;
}

int main() {
  struct sockaddr_in server_addr;
  socklen_t server_len = sizeof(server_addr);
  char buffer[BUF_SIZE], filename[BUF_SIZE];
  FILE *output_file;

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  // Create a UDP Socket
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  printf("Enter the file name to request from the server (format: "
         "<Roll_Number>_FileN.text): ");
  fgets(filename, BUF_SIZE, stdin);
  filename[strcspn(filename, "\n")] = '\0';

  // Sending the filename to the server
  send_message(sockfd, &server_addr, server_len, filename);

  // Receiving the words from the server
  int len = receive_message(sockfd, &server_addr, &server_len, buffer);

  // If an error message is received, print it and close the connection
  if (strncmp(buffer, "ERROR", 5) == 0) {
    printf("%s\n", buffer);
    close(sockfd);
    return 0;
  }
  // closing the connection if the file is not found
  if (strncmp(buffer, "NOTFOUND", 8) == 0) {
    printf("ERROR: %s\n", buffer);
    close(sockfd);
    return 0;
  }

  // Making a received_words.txt file to write the words received from the
  // server
  output_file = fopen("received_words.txt", "w");
  if (!output_file) {
    perror("Failed to create output file");
    close(sockfd);
    return 0;
  }

  // Writing the first word to the file  i.e "HELLO"
  fprintf(output_file, "%s\n", buffer);

  // Writing the rest of the words to the file
  while (1) {
    // Receiving the words from the server
    len = receive_message(sockfd, &server_addr, &server_len, buffer);
    if (strstr(buffer, "FINISH") != NULL) {
      fprintf(output_file, "%s\n", buffer);
      break;
    }
    fprintf(output_file, "%s\n", buffer);
  }

  // Closing the file and the connection and writing the completion message
  printf("File transfer complete. Words written to 'received_words.txt'\n");
  fclose(output_file);
  close(sockfd);

  return 0;
}
