/*
==============================================================
Name: Sumit Kumar
Roll no: 22CS30056
Lab Test 1-SetA
Chat Server Code
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
#define N 5
#define PORT 8080


int fn_set_fd(int fd,fd_set *master,int *max_fd){
    FD_SET(fd,master);
    if(fd>*max_fd){
        *max_fd=fd;
    }

    return 1;
}

int main(){

    struct sockaddr_in server_addr,client_addr;
    int clientsockfd[N];
    int connectedClient[N];
    int numclient=0;

    int server_fd;
    socklen_t server_len,client_len;

    client_len=sizeof(client_addr);

    for(int i=0;i<N;i++){
        connectedClient[i]=-1;
        clientsockfd[i]=-1;
    }

    if((server_fd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Socket conn failed ...\n");
        exit(1);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(server_fd,(struct sockaddr *)& server_addr,client_len)<0){
        perror("binding failed...\n");
        exit(1);
    }

    if(listen(server_fd,N)<0){
        perror("canot listen for conccurent conn");
        exit(1);
    }


    fd_set master,read_fd;
    int max_fd;
    max_fd=server_fd;
    char buf[BUF_SIZE];
    char send_buf[BUF_SIZE];
    memset(&buf,sizeof(buf),0);
    memset(&send_buf,sizeof(send_buf),0);

    while(1){
        read_fd=master;
        FD_ZERO(&read_fd);
        fn_set_fd(server_fd,&read_fd,&max_fd);

        int c=select(max_fd+1,&read_fd,NULL,NULL,NULL);

        if(numclient<2){
            perror("less than 2 clients in the server....\n");
            continue;
        } 


        for(int i=0;i<N;i++){
            if(connectedClient[i]==-1){
                if(FD_ISSET(clientsockfd[i],&read_fd)){
                    int con_fd=accept(server_fd,(struct sockaddr *)&client_addr,&client_len);
                    if(con_fd<0){
                        continue;
                    }
                    clientsockfd[i]=con_fd;
                    connectedClient[i]=1;
                    numclient++; 
                    fn_set_fd(con_fd,&read_fd,&max_fd);
                    
                     char *client_ip;
                    inet_aton(client_ip,&client_addr.sin_addr);

                    printf("Server: Received a new connection from client <%s: %d>",client_ip,client_addr.sin_port);
                }
            } else {
                if(FD_ISSET(clientsockfd[i],&read_fd)){
                    int bytes_read=recv(clientsockfd[i],buf,BUF_SIZE-1,0);
                    buf[bytes_read]='\0';
                    if(bytes_read<=0){
                        continue;
                    } 
                    else {
                         struct sockaddr_in f_addr;
                         socklen_t f_addr_len;

                             if(getpeername(clientsockfd[i],(struct sockaddr *)&f_addr,&f_addr_len)<0){
                                    perror("error  in fetching client ip by server....\n");
                            };
                        
                           char *client_ip;
                            inet_aton(client_ip,&f_addr.sin_addr);

                            if(numclient>2){
                                printf("Server: Insufficient clients,%s from client <%s: %d> dropped\n",buf,client_ip,f_addr.sin_port);
                                continue;
                            }
                        printf("Server: Received message %s from client <%s: %d>\n",buf,client_ip,f_addr.sin_port);

                        for(int i=0;i<N;i++){
                            if(connectedClient[i]==1){

                                memset(&send_buf,sizeof(send_buf),0);
                                char *new_buf;
                                
                                snprintf(new_buf,3*BUF_SIZE-1,"Client: Received Message %s from <%s:%d>",buf,client_ip,f_addr.sin_port);
                                send(clientsockfd[i],send_buf,BUF_SIZE-1,0);
                            }
                        }
                    }
                }
            }
        }
        



    }

    close(server_fd);

   
   return 0;
}