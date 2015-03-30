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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define BYTES_TO_REC 256

using namespace std;

int main(int argc, char **argv) {
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd < 0) {
		printf("There was a problem creating the socket\n");
		return 1;
	}
	  
	char serverPort[5000];
	char serverIP[5000];
	  
	printf("Enter server port number\n");
	fgets(serverPort,5000,stdin);
	  
	printf("Enter server IP address\n");
	fgets(serverIP,5000,stdin);
	  
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET; //9010
	serveraddr.sin_port = htons(atoi(serverPort));
	serveraddr.sin_addr.s_addr = inet_addr(serverIP);
	  
	printf("Enter a file name: ");
	char line[5000];
	fgets(line,5000,stdin);
	char *pos;
	if ((pos=strchr(line, '\n')) != NULL)
    	*pos = '\0';

	sendto(sockfd,line,strlen(line),0,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in));

	char * path;
	path = (char *)malloc(strlen(line)+strlen("client/")+1);
    	path[0] = '\0';   // ensures the memory is an empty string
    	strcat(path,"client/");
    	strcat(path,line);
	
	int bytesReceived = 0;
        char recvBuff[BYTES_TO_REC];

	socklen_t slen_server = sizeof(serveraddr);

	if (bytesReceived = recvfrom(sockfd, recvBuff, BYTES_TO_REC, 0, (struct sockaddr*)&serveraddr, &slen_server) > 0){  
		FILE * recFile;
		recFile = fopen(path, "w");
		
		printf("Bytes received %d\n", bytesReceived);
                fwrite(recvBuff, 1, bytesReceived, recFile);
		
		if(recFile != NULL){
			while(bytesReceived = recvfrom(sockfd, recvBuff, BYTES_TO_REC, 0, 					(struct sockaddr*)&serveraddr, &slen_server) > 0){
        			printf("Bytes received %d\n", bytesReceived);
        			fwrite(recvBuff, 1, bytesReceived, recFile);
    			}
			fclose(recFile);
			printf("Got from the server %s\n", line);
		}
	}	
    	else{
        	printf("Something went wrong reading the file from the server or the file does not exist\n");
  	}  
	close(sockfd);
  
  return 0;
}
