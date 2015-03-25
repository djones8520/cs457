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

#define BYTES_TO_REC 256

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
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
	serveraddr.sin_family = AF_INET;
	// 9876
	serveraddr.sin_port = htons(atoi(serverPort));
	// 148.61.162.118 (Changes depending on the machine where the server is running)
	serveraddr.sin_addr.s_addr = inet_addr(serverIP);
	  
	int e = connect(sockfd,(struct sockaddr*)&serveraddr,
	sizeof(struct sockaddr_in));
	  
	if (e < 0) {
		printf("There were some problems with connecting\n");
		return 1;
	}
	  
	printf("Enter a file name: ");
	char line[5000];
	//char line2[5000];
	fgets(line,5000,stdin);
	char *pos;
	if ((pos=strchr(line, '\n')) != NULL)
    	*pos = '\0';

	send(sockfd,line,strlen(line),0);

	char * path;
	path = malloc(strlen(line)+strlen("client/")+1);
    	path[0] = '\0';   // ensures the memory is an empty string
    	strcat(path,"client/");
    	strcat(path,line);
	
	int bytesReceived = 0;
        char recvBuff[BYTES_TO_REC];

	if((bytesReceived = read(sockfd, recvBuff, BYTES_TO_REC)) > 0){  
		FILE * recFile;
		recFile = fopen(path, "w");
		
		printf("Bytes received %d\n", bytesReceived);
                fwrite(recvBuff, 1, bytesReceived, recFile);
		
		if(recFile != NULL){
			while((bytesReceived = read(sockfd, recvBuff, BYTES_TO_REC)) > 0){
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
