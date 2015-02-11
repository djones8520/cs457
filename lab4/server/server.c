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

    char line[5000];
	while(1){		
		 //receive
		 printf("Receiving...\n");
		 if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&cli_temp, &slen_temp) > 0)  
			 printf("Receive error. \n");

		 if (clients <= max) {
			cli_addr[clients] = cli_temp;
			client_port[clients] = ntohs(cli_addr[clients].sin_port);
			clients++;
			printf("Client added\n");
			
			int i;
			for(i=0; i < max ;i++) {
				sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&cli_addr[i], sizeof(cli_addr[i]));

			}
		 }
	}

	return 0;
}
