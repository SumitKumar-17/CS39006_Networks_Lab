/* CS39006: Computer Networks Laboratory
 * A sample datagram socket client program
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 5000
#define MAXLINE 1000

int main() {
  char buffer[100];
  char *message = "Hello Server";
  int sockfd, n;
  struct sockaddr_in servaddr;

  // clear servaddr
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);
  servaddr.sin_family = AF_INET;

  // create datagram socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // request to send datagram
  sendto(sockfd, message, MAXLINE, 0, (struct sockaddr *)&servaddr,
         sizeof(servaddr));

  // waiting for response
  recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
  printf("\nReceived from Server:%s", buffer);

  // close the descriptor
  close(sockfd);
}
