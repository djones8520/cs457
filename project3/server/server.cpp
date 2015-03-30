/**********************************************************************************
* Project 3
* Due 4/1/2015
*
* Michael Kinkema
* Danny Selgo
* Daniel Jones
* Chase pietrangelo
*
**********************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define BYTES_TO_SEND 256
#define WINDOW_SIZE 5
#define BUFLEN 5000

void strip_newline(char* s);

using namespace std;

int main(int argc, char **argv)
{	
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd<0){
		printf("There was an error creating the socket\n");
		return 1;
	}
	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9010);
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	char buf[BUFLEN]; 

	socklen_t slen_client = sizeof(clientaddr);
	while(1){
		if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&clientaddr, &slen_client) < 0){  
			printf("Receive error. \n");
		}
		puts("Client connected");
		printf("Server received: %s\n",buf);
		FILE *fp = fopen(buf,"rb");
		if(fp == NULL){
			printf("Server: File read error\n");
			return 1;
		}

		while(1){
			char buff[BYTES_TO_SEND]={0};
			int bytesRead = fread(buff,1,BYTES_TO_SEND,fp);
			printf("Server: Bytes read %d\n",bytesRead);

			if(bytesRead > 0){
				puts("Server: Sending... \n");
				sendto(sockfd,buff,bytesRead,0,
					(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in));
			}

			if(bytesRead < BYTES_TO_SEND){
				if(feof(fp))
					puts("Server: Reached end of file");
				else if(ferror(fp))
					puts("Server: Error while reading file");
				break;
			}
		}
		fclose(fp);
	}
	close(sockfd);

	return 0;
}

void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}
