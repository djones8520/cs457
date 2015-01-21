
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv){

  	int sockfd = socket(AF_INET,SOCK_STREAM,0);
  	if(sockfd<0){
    		printf("There was an error creating the socket\n");
    		return 1;
  	}

	char name[5000];
	char serverPort[5000];
	char serverIP[5000]; 
 
	printf("Enter chat name: ");
	fgets(name,5000,stdin);

	printf("\nEnter server port number: ");
	fgets(serverPort,5000,stdin);
	  
	printf("\nEnter server IP address: ");
	fgets(serverIP,5000,stdin);
	printf("\n");

  	struct sockaddr_in serveraddr;
  	serveraddr.sin_family=AF_INET;
  	serveraddr.sin_port=htons(serverPort);
  	serveraddr.sin_addr.s_addr=inet_addr(serverIP);

  	if(connect(sockfd,(struct sockaddr*)&serveraddr,
                   sizeof(struct sockaddr_in))<0){
    		printf("There was an error connecting to the server.\n");
    		return 1;
  	}

  	printf("Connected to server. Welcome to the chat room!\n");
	
	while(1){
		
  		char line[5000];
		char msg[5000];
  		fgets(line,5000,stdin);
		strcat(msg,name);
		strcat(msg,": ");
		strcat(msg,line);
  		send(sockfd,msg,strlen(msg),0);
  		char line2[5000];
  		if(recv(sockfd,line2,5000,0) < 0){
    			printf("Sorry, had a problem receiving.\n");
    			return 1;
  		}
  		printf("%s\n",line2);
	}
  	return 0;
}
