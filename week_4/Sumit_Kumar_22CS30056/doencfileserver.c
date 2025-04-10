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

/*
 * Encrypts the input file using the given key and writes the output to the
 * output file. The key is a 26-character string where each character represents
 * the substitution for the corresponding letter in the alphabet.
 */
void encrypt_file(const char *input_file, const char *output_file,
                  const char *key) {
  FILE *fin = fopen(input_file, "r");
  FILE *fout = fopen(output_file, "w");

  if (!fin || !fout) {
    perror("File operation failed");
    exit(1);
  }

  int c;
  /*
   * Read each character from the input file and write the corresponding
   * substitution to the output file. Preserve the original case of the
   * character.
   */
  while ((c = fgetc(fin)) != EOF) {
    if (isalpha(c)) {
      int is_upper = isupper(c);
      int index = toupper(c) - 'A';
      char encrypted = key[index];
      /*
       * Preserve the original case as in the input file.
       */
      if (is_upper) {
        encrypted = toupper(encrypted);
      } else {
        encrypted = tolower(encrypted);
      }
      fputc(encrypted, fout);
    } else {
      /*
       * If the character is not an alphabet, write it as is to the output file.
       */
      fputc(c, fout);
    }
  }

  fclose(fin);
  fclose(fout);
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE];
  char key[27];

  /*
   * Create socket for the server.
   */
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket creation failed");
    exit(1);
  }

  int opt = 1;
  /*
   * Allow the server to reuse the address and port.
   * This is useful when the server is restarted and the port is still in use.
   */
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("Setsockopt failed");
    exit(1);
  }

  /*
   * Configure server address and port.
   */
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  /*
   * Bind the server to the address and port.
   * This is necessary to listen for incoming connections.
   */
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    exit(1);
  }

  /*
   * Listen for incoming connections.
   * The server can handle up to 5 connections in the queue.
   */
  if (listen(server_fd, 5) < 0) {
    perror("Listen failed");
    exit(1);
  }

  printf("Server listening on port %d...\n", PORT);

  while (1) {
    /*
     * Accept incoming connection from a client.
     * This is a blocking call and will wait until a client connects.
     */
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                            &client_len)) < 0) {
      perror("Accept failed");
      continue;
    }

    /*
     * Get the IP address and port of the client.
     * The INET_ADDRSTRLEN is the maximum length of an IPv4 address.
     * The inet_ntop function converts the IP address from binary to text form.
     * The ntohs function converts the port number from network byte order to
     * host byte order.
     */
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    printf("Client connected: %s:%d\n", client_ip, client_port);

    while (1) {
      /*
       * Receive the encryption key from the client.
       * The key is a 26-character string where each character represents the
       * substitution for the corresponding letter in the alphabet.
       */
      memset(key, 0, sizeof(key));
      ssize_t key_bytes = recv(client_fd, key, 26, 0);
      if (key_bytes <= 0) {
        break;
      }
      key[26] = '\0';

      /*
       * Create a temporary file to store the content received from the client.
       * The filename is based on the client's IP address and port number.
       */
      char temp_filename[BUFFER_SIZE];
      snprintf(temp_filename, BUFFER_SIZE, "%s.%d.txt", client_ip, client_port);

      /*
       * Receive the file content from the client in chunks.
       * The end of file is marked by a triple newline.
       */
      FILE *temp_file = fopen(temp_filename, "w");
      if (!temp_file) {
        perror("Failed to create temporary file");
        break;
      }

      /*
       * Read the content from the client in chunks and write it to the
       * temporary file. Check for the end of file marker.
       */
      ssize_t bytes_received;
      int end_of_file = 0;
      while (!end_of_file && (bytes_received = recv(client_fd, buffer,
                                                    BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        /*
         * Check for the end of file marker.
         * If the marker is found, truncate the content at the marker.
         * I am using "###EOF###" as the end of file marker.
         */
        if (strstr(buffer, "###EOF###")) {
          *strstr(buffer, "###EOF###") = '\0';
          end_of_file = 1;
        }
        fputs(buffer, temp_file);
      }
      /*
       * Close the temporary file after writing the content.
       * THe complete file is received from the client  then the file encrypted
       * and sent back to the client.
       */
      fclose(temp_file);

      char enc_filename[BUFFER_SIZE];
      size_t base_len = strlen(temp_filename);
      if (base_len > BUFFER_SIZE - 5) {
        // Check if we have room for ".enc\0"
        base_len = BUFFER_SIZE - 5;
      }
      strncpy(enc_filename, temp_filename, base_len);
      strcpy(enc_filename + base_len, ".enc");

      /*
       * Encrypt the temporary file using the key provided by the client.
       * The encrypted content is written to a new file.
       * The encrypt function is defined above.
       */
      encrypt_file(temp_filename, enc_filename, key);

      /*
       * Send the encrypted file back to the client
       */
      FILE *enc_file = fopen(enc_filename, "r");
      if (!enc_file) {
        perror("Failed to open encrypted file");
        break;
      }

      /*
       * Read the encrypted content from the file in chunks and send it to the
       * client. The client will receive the content in chunks and write it to a
       * file.
       */
      while (fgets(buffer, BUFFER_SIZE - 1, enc_file)) {
        send(client_fd, buffer, strlen(buffer), 0);
      }

      /*
       * Send the end of file marker to the client.
       * The client will use this marker to identify the end of the file.
       * I am using "###EOF###" as the end of file marker.
       * I have a special length of 9 characters for this specially
       */
      send(client_fd, "###EOF###", 9, 0);

      /*
       * File Sending is done, close the file.
       */
      fclose(enc_file);

      /*
       * Remove the two temporary files created during the process.
       */
      remove(temp_filename);
      remove(enc_filename);

      /*
       * Ask the client if they want to encrypt another file.
       * If the client responds with "No", break the loop and close the
       * connection.
       */
      memset(buffer, 0, BUFFER_SIZE);
      if (recv(client_fd, buffer, BUFFER_SIZE - 1, 0) <= 0 ||
          strcmp(buffer, "No") == 0) {
        break;
      }
    }

    close(client_fd);
    printf("Client disconnected: %s:%d\n", client_ip, client_port);
  }

  close(server_fd);
  return 0;
}