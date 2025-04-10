/*
=====================================================================================================================
=   Assignment 3 Submission = =   Name: Sumit Kumar = =   Roll number: 22CS30056
= =   Link of the pcap file:
https://drive.google.com/file/d/1JCuvw6KFdM5y-s1ioQ2Q-RqYY9OqKxyd/view?usp=sharing
=
=====================================================================================================================
*/

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 100
#define SERVER_IP "127.0.0.1"

/*
 * Validate the encryption key provided by the user.
 * The key must be exactly 26 characters long and contain only alphabets.
 * Validate the key first before sending it to the server.
 */
int validate_key(const char *key) {
  if (strlen(key) != 26) {
    return 0;
  }
  for (int i = 0; i < 26; i++) {
    if (!isalpha(key[i])) {
      return 0;
    }
  }
  return 1;
}

int main() {
  int sock_fd;
  struct sockaddr_in server_addr;
  char buffer[BUFFER_SIZE];
  char filename[BUFFER_SIZE];
  char key[27];

  /*
   * Create a socket for communication with the server.
   * The socket function returns a file descriptor that can be used in later
   * system calls.
   */
  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation failed");
    exit(1);
  }

  /*
   * Configure the server address and port.
   * The server address is the IP address of the server.
   * The server port is the port number on which the server is listening.
   */
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
    perror("Invalid address");
    exit(1);
  }

  /*
   * Connect to the server using the socket and server address.
   * The connect function establishes a connection to the server.
   */
  if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Connection failed");
    exit(1);
  }

  printf("Connected to server\n");

  while (1) {
    /*
     * Get the filename from the user.
     * The filename is the name of the file to be encrypted.
     * The file must exist in the current directory.
     */
    while (1) {
      printf("Enter filename to encrypt: ");
      scanf("%99s", filename);

      FILE *file = fopen(filename, "r");
      if (file) {
        fclose(file);
        break;
      }
      printf("NOTFOUND %s\n", filename);
    }

    /*
     * Sending the encryption key to the server.
     * This is an infinte loop that will continue until a valid key is provided.
     */
    while (1) {
      printf("Enter 26-character key: ");
      scanf("%26s", key);
      key[26] = '\0';

      if (validate_key(key)) {
        break;
      }
      printf("Invalid key. Key must be exactly 26 letters.\n");
    }

    /*
     * Encryption key is sent to the server.
     */
    send(sock_fd, key, 26, 0);

    /*
     * Send the contents of the file to the server.
     * The file is read in chunks and sent to the server.
     * The server will receive the content in chunks and process accordingly.
     */
    FILE *file = fopen(filename, "r");
    while (fgets(buffer, BUFFER_SIZE - 1, file)) {
      send(sock_fd, buffer, strlen(buffer), 0);
    }

    /*
     * Send the end of file marker to the server to indicate the end of the
     * file.
     */
    send(sock_fd, "###EOF###", 9, 0);
    fclose(file);

    /*
     * Receive the encrypted file from the server.
     * The server will send the encrypted file in chunks.
     * The client will receive the content in chunks and write it to a file.
     */
    char enc_filename[BUFFER_SIZE];
    size_t base_len = strlen(filename);
    if (base_len > BUFFER_SIZE - 5) {
      // Check if we have room for ".enc\0"
      base_len = BUFFER_SIZE - 5;
    }
    strncpy(enc_filename, filename, base_len);
    strcpy(enc_filename + base_len, ".enc");

    /*
     * Create a new file to write the encrypted content.
     */
    FILE *enc_file = fopen(enc_filename, "w");
    if (!enc_file) {
      perror("Failed to create encrypted file");
      break;
    }

    /*
     * Receive the encrypted content in chunks and write it to the file.
     * The end of file marker is used to identify the end of the file.
     * My end of file marker is "###EOF###".
     * I have a special length of 9 characters for this specially
     */
    ssize_t bytes_received;
    int end_of_file = 0;
    while (!end_of_file &&
           (bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
      buffer[bytes_received] = '\0';
      if (strstr(buffer, "###EOF###")) {
        *strstr(buffer, "###EOF###") = '\0';
        end_of_file = 1;
      }
      fputs(buffer, enc_file);
    }

    fclose(enc_file);

    printf("File encrypted successfully!\n");
    printf("Original file: %s\n", filename);
    printf("Encrypted file: %s.enc\n", filename);

    /*
     * Ask the user if they want to encrypt another file.
     * If the user responds with "No", the client will exit.
     */
    printf("Do you want to encrypt another file? (Yes/No): ");
    char response[4];
    scanf("%3s", response);

    /*
     * Send the response to the server.
     * If the response is "No", the client will exit.
     */
    send(sock_fd, response, strlen(response), 0);

    if (strcmp(response, "No") == 0) {
      break;
    }
  }

  close(sock_fd);
  return 0;
}