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
#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

void* httpRequest(void* arg);

int main(int argc, char **argv){
	int port;
	char *docroot = (char*)malloc(1024);
	char *logfile;

	port = 8080;
	getcwd(docroot,1024);
	if (argc > 1){
		for (int i = 1; i < argc; i++){
			if (strcmp(argv[i], "-p") == 0){
				i++;
				port = atoi(argv[i]);
			}
			else if (strcmp(argv[i], "-docroot") == 0){
				i++;
				docroot = argv[i];
			}
			else if (strcmp(argv[i], "-logfile") == 0){
				i++;
				logfile = argv[i];
				freopen(argv[i], "w", stdout);
			}
			else{
				cerr << "Invalid option. Valid options are -p, -docroot, -logfile\n";
				return 1;
			}
		}
	}
	printf("port: %d\n",port);
	printf("docroot: %s\n",docroot);
	printf("logfile: %s\n",logfile);
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0){
		printf("Problem creating socket\n");
		return 1;
	}

	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9010);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
	listen(sockfd, 10);

	char line[5000];
	while (1){
		pthread_t thread;
		void *result;
		int status;

		socklen_t len = sizeof(clientaddr);
		//g++ doesn't like this
		int clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);
		   
		char line[5000];
		recv(clientsocket, line, 5000, 0);
		printf("Requested file from client: %s\n",line);

		//char str[INET_ADDRSTRLEN];
		//inet_ntop(AF_INET, &clientaddr, &str, INET_ADDRSTRLEN);
		//printf("A client connected (IP=%s : Port=9010)\n", str);

		if((status = pthread_create(&thread, NULL, httpRequest, &line)) != 0){
			fprintf(stderr, "thread create error %d: %s\n", status, strerror(status));
		}
	}

	return 0;
}

void* httpRequest(void* arg){
	//char line[5000];
	//int sockfd = *(int *) arg;
        //int n;

	cout << "Thread created";
/*
        while((n = recv(sockfd,line,5000,0))>0){
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
*/
	//close(sockfd);
	pthread_detach(pthread_self());
}


