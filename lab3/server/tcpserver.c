#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>

int main(int argc, char **argv){
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

	while(1){
		fd_set tmp_set = sockets;
		select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);

		int i;
		for(i=0; i<FD_SETSIZE; i++){
			if(FD_ISSET(i, &tmp_set)){
				if(i == sockfd){
					printf("A client connected\n");
					int clientsocket = accept(sockfd, (struct sockaddr*) &clientaddr, &len);
					FD_SET(clientsocket, &sockets);
				}
				else{
                    printf("here 1\n");
                    char line[5000];
                    //memset(line,'\0',5000);
                    printf("here 2\n");
                    recv(i, line, 5000, 0);
					printf("Got from client: %s\n", line);
					printf("here 3\n");
					int j;
					printf("here 4\n");
					for(j=4; j<FD_SETSIZE; j++){
                        if(j != i)
                            send(j, line, strlen(line), 0);
                    }
					printf("here 5\n");
					
					//close(i);
					//FD_CLR(i, &sockets);
				}
			}
		}


		
	}

	return 0;
}

