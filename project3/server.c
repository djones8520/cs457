/**********************************************************************************
* Lab 2
* Due 1/21/2015
*
* Michael Kinkema
* Danny Selgo
* Daniel Jones
* Chase pietrangelo
*
**********************************************************************************/


#define BYTES_TO_SEND 256
#define WINDOW_SIZE 5

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

void strip_newline(char* s);

typedef struct {
    struct sockaddr_in serveraddr;
    int sockfd;
}requestParams;

int main(int argc, char **argv)
{
	//Create the socket
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd<0){
			printf("There was an error creating the socket\n");
			return 1;
	}

	//Get server info
	char serverPort[5000];
	char serverIP[5000];

	printf("Enter server port number: ");
	fgets(serverPort,5000,stdin);

	printf("Enter server IP address: ");
	fgets(serverIP,5000,stdin);

	struct sockaddr_in serveraddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(atoi(serverPort));
	serveraddr.sin_addr.s_addr=inet_addr(serverIP);

	//Attempt to connect to server
	char hello[] = "/hello";
	strip_newline(hello);
	sendto(sockfd,hello,strlen(hello),0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	char connect[5000];
		recvfrom(sockfd,connect,5000,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	if(strcmp(connect,hello)==0){
		printf("Connected to server. Welcome to the chat room!\n");
	}else{
		printf("Unable to connect to server.\n");
		exit(0);
	}

	pthread_t thread1;
	void *result1;
	int status;
	requestParams req;
    	req.serveraddr = serveraddr;
    	req.sockfd = sockfd;
	if((status = pthread_create(&thread1, NULL, print_message, &req)) != 0){
		fprintf(stderr, "thread create error %d: %s\n", status, strerror(status));
	}


/*
	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9876);
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	listen(sockfd,10);
	while(1){
		int len = sizeof(clientaddr);
		int clientsocket =
			accept(sockfd,(struct sockaddr*)&clientaddr,&len);
		puts("Client connected");
		char line[5000];
		recv(clientsocket,line,5000,0);
		printf("Server received: %s\n",line);
		//send(clientsocket, line, 5000,0);
		FILE *fp = fopen(line,"rb");
		if(fp == NULL)
		{
			printf("Server: File read error\n");
			return 1;
		}

		while(1)
		{
			char buff[BYTES_TO_SEND]={0};
			int bytesRead = fread(buff,1,BYTES_TO_SEND,fp);
			printf("Server: Bytes read %d\n",bytesRead);

			if(bytesRead > 0)
			{
				puts("Server: Sending... \n");
				send(clientsocket,buff,bytesRead,0);
			}

			if(bytesRead < BYTES_TO_SEND)
			{
				if(feof(fp))
					puts("Server: Reached end of file");
				else if(ferror(fp))
					puts("Server: Error while reading file");
				break;
			}
		}
		close(clientsocket);
		fclose(fp);
	}

	return 0;
	*/
}

void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}
