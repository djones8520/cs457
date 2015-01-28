/****************************************************************
* Project 1: Mini HTTP Server
* Due 2/4/2015
*
* Server File
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
#include <string.h>
#include <sys/select.h>
#include <pthread.h>

void* httpRequest(void* arg);

int main(int argc, char **argv){
	int p;
	String docroot;
	String logfile;
	
	if(argc > 1){
		
	}else{

	}

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        fd_set sockets;
        FD_ZERO(&sockets);

        if(sockfd<0){
                printf("Problem creating socket\n");
                return 1;
        }

        struct sockaddr_in serveraddr, clientaddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port=htons(9010);
        serveraddr.sin_addr.s_addr = INADDR_ANY;

        bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
        listen(sockfd, 10);

        FD_SET(sockfd, &sockets);

        int len = sizeof(clientaddr);
	
	pthread_t thread1;
	void *result1;
	int status;

	// NOT IN THE RIGHT PLACE
	if((status = pthread_create(&thread1, NULL, httpRequest, &sockfd)) != 0){
		fprintf(stderr, "thread create error %d: %s\n", status, strerror(status));
	}

    	char line[5000];
        while(1){
                fd_set tmp_set = sockets;
                select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);

                int i;
                for(i=0; i<FD_SETSIZE; i++){
                        if(FD_ISSET(i, &tmp_set)){
                                if(i == sockfd){

                    char str[INET_ADDRSTRLEN];


                    int clientsocket = accept(sockfd, (struct sockaddr*) &clientaddr, &len);
                                        FD_SET(clientsocket, &sockets);
                        inet_ntop( AF_INET, &clientaddr, &str, INET_ADDRSTRLEN );
                        printf("A client connected (IP=%s : Port=9010)\n",str);
                                }
                                else{
                    if(recv(i, line, 5000, 0)) {
                        printf("Got from client: %s\n", line);
                        int j;
                        for(j=4; j<FD_SETSIZE; j++){
                            if(j != i)
                                send(j, line, strlen(line), 0);
                        }
                        memset(line,0,sizeof(line));
                    } else {
                        puts("Client disconnected.");
                        close(i);
                        FD_CLR(i, &sockets);
                    }
                                }
                        }
                }
        }

	if ((status = pthread_join (thread1, &result1)) != 0) { 
        	fprintf (stderr, "join error %d: %s\n", status, strerror(status)); 
        	exit (1); 
    	}

        return 0;
}

void* httpRequest(void* arg){
	char line[5000];
	int sockfd = *(int *) arg;
	pthread_detach(pthread_self());
}


