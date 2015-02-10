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

struct requestParams{
    int serveraddr;
    int sockfd;
};

int main(int argc, char** argv){

  	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
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

	struct timeval to;
  	to.tv_sec = 5;
  	to.tv_usec = 0;

  	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to));

  	struct sockaddr_in serveraddr;
  	serveraddr.sin_family=AF_INET;
  	serveraddr.sin_port=htons(atoi(serverPort));
  	serveraddr.sin_addr.s_addr=inet_addr(serverIP);

  	printf("Connected to server. Welcome to the chat room!\n");

	pthread_t thread1;
	void *result1;
	int status;
	
	requestParams req = malloc(sizeof(requestParams));
        
    string requestData = line;
    req->serveraddr = serveraddr;
    req->sockfd = sockfd;

	if((status = pthread_create(&thread1, NULL, print_message, &req)) != 0){
		fprintf(stderr, "thread create error %d: %s\n", status, strerror(status));
	}

	while(1){
        char line[5000];
        char msg[5000];
        
        fgets(line,5000,stdin);
		
        strip_newline(name);
        strip_newline(line);
        
        if (strcmp(line,"/exit") == 0) {
            exit(0);
        }
        
        strcat(msg,name);
        strcat(msg,": ");
	strip_newline(msg);
        strcat(msg,line);
	strip_newline(msg);

        sendto(sockfd,msg,strlen(msg),0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
        
        memset(line,0,sizeof(line));
        memset(msg,0,sizeof(msg));
    }

	if ((status = pthread_join (thread1, &result1)) != 0) { 
        	fprintf (stderr, "join error %d: %s\n", status, strerror(status)); 
        	exit (1); 
    	} 
        return 0;
}

void* print_message(void* arg){
	char line[5000];
	int sockfd = *(int *) arg;
        int n;
        while(n = recvfrom(sockfd,line,5000,0,(struct sockaddr*)&serveraddr,sizeof(serveraddr))>0){
            printf("%s\n",line);
            memset(line,0,sizeof(line));
        }
        if(n<0){
            printf("Sorry, had a problem receiving.\n");
            exit(1);
        } else {
            printf("Server has disconnected.\n");
            exit(1);
        }
	close(sockfd);
	pthread_detach(pthread_self());
}

void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}
