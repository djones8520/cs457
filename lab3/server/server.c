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

    char line[5000];
	while(1){
		fd_set tmp_set = sockets;
		select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);

		int i;
		for(i=0; i<FD_SETSIZE; i++){
			if(FD_ISSET(i, &tmp_set)){
				if(i == sockfd){
					
                    char str[INET_ADDRSTRLEN];
                    inet_ntop( AF_INET, &clientaddr, &str, INET_ADDRSTRLEN );
                    printf("A client connected (IP=%s : Port=9010)\n",str);
                    
                    int clientsocket = accept(sockfd, (struct sockaddr*) &clientaddr, &len);
					FD_SET(clientsocket, &sockets);
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

	return 0;
}
