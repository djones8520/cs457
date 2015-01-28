/****************************************************************
* Lab 3: Mini Chat Server
* Due 1/28/2015
*
* Client File
*    
* Michael Kinkema
* Chase	Pietrangelo
* Danny	Selgo
* Daniel Jones
*
****************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void* print_message(void* arg);
void strip_newline(char* s);

int main(int argc, char** argv){

  	int sockfd = socket(AF_INET,SOCK_STREAM,0);
  	if(sockfd<0){
    		printf("There was an error creating the socket\n");
    		return 1;
  	}

	char name[5000];
	char serverPort[5000];
	char serverIP[5000]; 

	//if(argc!=4){ 
		
		printf("Enter chat name: ");
		fgets(name,5000,stdin);

		printf("Enter server port number: ");
		fgets(serverPort,5000,stdin);
	  
		printf("Enter server IP address: ");
		fgets(serverIP,5000,stdin);
		
	/*}else{
		name=argv[1];
		serverPort=argv[2];
		serverIP=argv[3];
	}*/

  	struct sockaddr_in serveraddr;
  	serveraddr.sin_family=AF_INET;
  	serveraddr.sin_port=htons(atoi(serverPort));
  	serveraddr.sin_addr.s_addr=inet_addr(serverIP);

  	if(connect(sockfd,(struct sockaddr*)&serveraddr,
                   sizeof(struct sockaddr_in))<0){
    		printf("There was an error connecting to the server.\n");
    		return 1;
  	}

  	printf("Connected to server. Welcome to the chat room!\n");

	pthread_t wrt_thread1;
	pthread_t rd_thread

	pthread_create(&wrt_thread,NULL, print_message,"The thread is listening!\n");
	pthread_create(&rd_thread,NULL, read_message, "Reading the user input.\n");

	int status;

	while(1){
                char line[5000];
                char msg[5000];
                fgets(line,5000,stdin);
		strip_newline(name);
                strcat(msg,name);
                strcat(msg,": ");
		strip_newline(msg);
                strcat(msg,line);
		strip_newline(msg);
                send(sockfd,msg,strlen(msg),0);
        }

        return 0;
}

void* print_message(void* arg){
	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
  	listen(sockfd,10);
  	FD_SET(sockfd,&sockets);

  	int len=sizeof(clientaddr);
  	while(1){
    		fd_set tmp_set = sockets;
    		select(FD_SETSIZE,&tmp_set,NULL,NULL,NULL);
    		int i;
    		for(i=0; i<FD_SETSIZE; i++){
      			if(FD_ISSET(i,&tmp_set)){
				if(i==sockfd){
	  				printf("A client connected\n");
	  				int clientsocket = accept(sockfd,
				    	(struct sockaddr*)&clientaddr,
				    	&len);
	  				FD_SET(clientsocket,&sockets);
				} else {
	  				char line[5000];
	  				recv(i,line,5000,0);
	  				printf("Got from client: %s\n",line);
	  				send(i,line,strlen(line),0);
	  				close(i);
	  				FD_CLR(i,&sockets);
				}
      			}
    		}
	}
}

void* read_message(void* arg)
{
	int status;

        while(1){
                char line[5000];
                char msg[5000];
                fgets(line,5000,stdin);
                strip_newline(name);
                strcat(msg,name);
                strcat(msg,": ");
                strip_newline(msg);
                strcat(msg,line);
                strip_newline(msg);
                send(sockfd,msg,strlen(msg),0);
        }
}

void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}
