/*
==============================================================
Name: Sumit Kumar
Roll no: 22CS30056
Lab Test 1-SetA
ChatClient code
==============================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>


#define BUF_SIZE 1024
#define PORT 8080
#define SERVER_IP "127.0.0.1"


int fn_set_fd(int fd,fd_set *master,int *max_fd){
    FD_SET(fd,master);
    if(fd>*max_fd){
        *max_fd=fd;
    }

    return 1;
}



int main(){
   

    struct sockaddr_in server_addr;
    socklen_t server_len;
    int sock_fd;

    if((sock_fd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Socket conn failed ...\n");
        exit(1);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=inet_addr(SERVER_IP);

    if((connect(sock_fd,(struct sockaddr * )&server_addr,sizeof(server_addr)))<0){
        perror("Error in client Connection...\n");
        exit(1);
    }




    fd_set master;
    int max_fd;

    char user_input[BUF_SIZE];
    char buf[BUF_SIZE];

    int nread=0;

    max_fd=(STDIN_FILENO> sock_fd)?STDIN_FILENO:sock_fd;

    while(1){
        FD_ZERO(&master);
        fn_set_fd(STDIN_FILENO,&master,&max_fd);
        fn_set_fd(sock_fd,&master,&max_fd);

        select(max_fd+1,&master,NULL,NULL,NULL);

        if(FD_ISSET(STDIN_FILENO,&master)){

            printf("hello");
            nread=read(STDIN_FILENO,user_input,BUF_SIZE-1);
            // get(user_input,BUF_SIZE-1);
            user_input[nread]='\0';
            // printf("hello testing\n");

            char *client_ip;
            socklen_t f_addr_len;
            struct sockaddr_in f_addr;
            inet_aton(client_ip,&f_addr.sin_addr);

            if(getpeername(sock_fd,(struct sockaddr *)&f_addr,&f_addr_len)<0){
                perror("error  in fetching client ip by server....\n");
            };

            send(sock_fd,user_input,BUF_SIZE-1,0);
            printf("Client <%s: %d> Message <%s> sent to the server",client_ip,f_addr.sin_port,buf);
        } 


        if(FD_ISSET(sock_fd,&master)){
            int byte_read=recv(sock_fd,buf,BUF_SIZE-1,0);
            if(byte_read<=0) continue;
            buf[byte_read]='\0';

            // char *client_ip;
            // struct sockaddr_in f_addr;
            // socklen_t f_addr_len;
            // inet_aton(client_ip,&client_addr.sin_addr);

            
            // if(getpeername(sock_fd,(struct sockaddr *)&f_addr,&f_addr_len)<0){
            //     perror("error  in fetching client ip by server....\n");
            // };
            // memset(&buf,sizeof(buf),0);

           int rread=recv(sock_fd,buf,BUF_SIZE-1,0);
           buf[rread]='\0';
           printf("%s",buf);



        }
    }

    close(sock_fd);
    





















   return 0;
}