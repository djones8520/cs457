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
#include <string>
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
		int total = 0;
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
			char readbuff[BYTES_TO_SEND - 3];
			char header[3]={'0','0','0'};
			int bytesRead = fread(readbuff,1,BYTES_TO_SEND - 3,fp);
			total+= bytesRead;
			
			//if(bytesRead < BYTES_TO_SEND - 3){
			if(bytesRead > 0){
				if(feof(fp)){
					header[2] = '1';
				}else if(ferror(fp)){
					puts("Server: Error while reading file");
				}
			}else if(bytesRead == 0){
				header[2] = '1';
			}

			char sendbuff[BYTES_TO_SEND];
			memcpy(sendbuff,header,3);
			memcpy(&sendbuff[3],readbuff,bytesRead);

			//printf("Server: BytesRead %d\n",bytesRead);
			//printf("Server: SendBuff Size... %d\n",strlen(sendbuff));
			//printf("Server: sent %s\n",&sendbuff[3]);
			sendto(sockfd,sendbuff,bytesRead + 3,0,
				(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in));
			
			if(bytesRead <= 0){
				puts("Server: Reached end of file");
				break;			
			}

		}
		printf("File sent.  Total Bytes... %d\n",total);
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
