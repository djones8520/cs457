#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <arpa/inet.h>

int max = 10;
int BUFLEN = 5000;

int main(int argc, char **argv){
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in my_addr, cli_addr[max],cli_temp;  
	socklen_t slen[max],slen_temp;

	slen_temp = sizeof(cli_temp);
	char buf[BUFLEN];  
	int clients = 0;
	int client_port[max];
	
	if(sockfd<0){
		printf("Problem creating socket\n");
		return 1;
	}

	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port=htons(9010);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
	printf("Receiving...\n");

	while(1){
		if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&cli_temp, &slen_temp) < 0){  
			 printf("Receive error. \n");
		}

		if(strcmp(buf,"/hello")==0){
			
		}else if(strcmp(buf,"/exit")==0){
		
		}else{
			int check = 1;
			int j;
			for(j = 0; j < max; j++){
				if(cli_addr[j].sin_addr.s_addr  == cli_temp.sin_addr.s_addr)
					check = 0; 
			}               

		 	if (clients <= max) {
				if(check){
					cli_addr[clients] = cli_temp;
					client_port[clients] = ntohs(cli_addr[clients].sin_port);
					clients++;
				}

				printf("Received from client (IP=%s : Port=9010):%s\n", inet_ntoa(cli_temp.sin_addr),buf);

			
				int i;
				for(i=0; i < max ;i++) {
					sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&cli_addr[i], sizeof(cli_addr[i]));
				}

				memset(buf, 0, sizeof(buf));
		 	}
		}
	}
	return 0;
}
