//多线程并发服务器
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "send_msg.h"

#define PORT 1111
#define BACKLOG 5

typedef struct {
    int connectfd;
    struct sockaddr_in client;
}targs;
/*
void *function(void* arg)  
{  
struct ARG *info;  
info = (struct ARG*)arg;  
process_cli(info->connfd,info->client);  
free (arg);  
pthread_exit(NULL);  
}  
*/
void *process(void *arg){
    targs client;
    client.connectfd=((targs *)arg)->connectfd;
    client.client=((targs *)arg)->client;

    char buf[1024];

    int n=0;
    while(0<(n=read(client.connectfd,buf,sizeof(buf)))){
        buf[n]='\0';
        printf("You got a message <%s> from %s.\n",buf,inet_ntoa(client.client.sin_addr));
	/*
        int i=0;
        while(buf[i]!='\0'&&i<1024){
            if(buf[i]>='a'&&buf[i]<='z')
                buf[i]+=('A'-'a');
            i++;
			
        }
		*/
		//处理并发送接收的消息
		process_received(buf);
        //write(client.connectfd,buf,strlen(buf));
    }
    close(client.connectfd);
	//free (arg);  
	//pthread_exit(NULL); 
};




int main()
{
    int listenfd,connectfd;
    struct sockaddr_in server;
    struct sockaddr_in client;

    if(-1==(listenfd=socket(AF_INET,SOCK_STREAM,0))){
        perror("create socket error\n");
        exit(1);
    }

    int opt=SO_REUSEADDR;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    bzero(&server,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(PORT);
    server.sin_addr.s_addr=htonl(INADDR_ANY);

    if(-1==(bind(listenfd,(struct sockaddr*)&server,sizeof(struct sockaddr)))){
        perror("bind error\n");
        exit(1);
    }

    if(-1==(listen(listenfd,BACKLOG))){
        perror("listen error\n");
        exit(1);
    }

    int len=sizeof(client);
    targs arg;

    while(1){
        if(-1==(connectfd=accept(listenfd,(struct sockaddr *)&client,&len))){
            perror("accept error\n");
            exit(1);
        }
        arg.connectfd=connectfd;
        arg.client=client;

        pthread_t tid;
        if(pthread_create(&tid,NULL,process,(void *)&arg)){
            perror("create thread error\n");
            exit(1);
        }
    }
    close(listenfd);
    return 0;
}