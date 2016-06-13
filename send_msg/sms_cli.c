#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 1111
#define MAXDATASIZE 1024


int main(int argc,char **argv){
    int sockfd;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in server;
    if(2>argc){
        fprintf(stderr,"Usage: %s \n",argv[0]);
        exit(1);
    }

    if(NULL==(he=gethostbyname(argv[1]))){
        perror("gethostbyname error\n");
        exit(1);
    }

    if(-1==(sockfd=socket(AF_INET,SOCK_STREAM,0))){
        perror("socket error\n");
        exit(1);
    }

    bzero(&server,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(PORT);
    server.sin_addr = *((struct in_addr*)he->h_addr);
    if(-1==connect(sockfd,(struct sockaddr*)&server,sizeof(struct sockaddr))){
        perror("connect error\n");
        exit(1);
    }

    while(1){
        gets(buf);
        write(sockfd,buf,strlen(buf));
        printf("send %s to server\n",buf);
        int num=read(sockfd,buf,sizeof(buf));
        buf[num]='\0';
        puts(buf);
    }
    close(sockfd);
    return 0;
}