#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <arpa/inet.h>

int main(int argc, char **argv){
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

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

	FD_SET(sockfd, &sockets);

    char line[5000];
	while(1){
		int len = sizeof(clientaddr);
		fd_set tmp_set = sockets;
		select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);

		int i;
		for(i=0; i<FD_SETSIZE; i++){
			if(FD_ISSET(i, &tmp_set)){
				if(i == sockfd){
					
                    char str[INET_ADDRSTRLEN];
                    
			//inet_ntop( AF_INET, &clientaddr, &str, INET_ADDRSTRLEN );
                    	//int len;
		
			//len = sizeof(clientaddr);
			//getpeername(i,&clientaddr,&len);
			//str = inet_ntoa(clientaddr.sin_addr);

			printf("A client connected (IP=%s : Port=9010)\n", inet_ntoa(clientaddr.sin_addr));
				}
				else{
                    if(recvfrom(i, line, 5000, 0,(struct sockaddr*)&clientaddr,&len)){
                        printf("Got from client: %s\n", line);
                        int j;
                        for(j=4; j<FD_SETSIZE; j++){
                            if(j != i)
                                sendto(j, line, strlen(line), 0,(struct sockaddr*)&clientaddr,&len);
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
