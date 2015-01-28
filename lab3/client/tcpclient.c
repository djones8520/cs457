#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv){
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd<0){
		printf("Problem creating socket\n");
		return 1;
	}
/*
	char serverPort[5000];
	char serverIP[5000];
	  
	printf("Enter server port number\n");
	fgets(serverPort,5000,stdin);
	  
	printf("Enter server IP address\n");
	fgets(serverIP,5000,stdin);
	*/
	
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	// 9010 atoi(serverPort)
	serveraddr.sin_port=htons(9010);
	// 148.61.162.118 (Changes depending on the machine where the server is running) serverIP
	serveraddr.sin_addr.s_addr=inet_addr("148.61.162.118");

	int e = connect(sockfd, (struct sockaddr*) &serveraddr, sizeof (struct sockaddr_in));

	if(e<0){
		printf("Error in connecting\n");
		return 1;
	}

	while(1){
		printf("Enter a line: ");
		char line[5000];
		char line2[5000];

		fgets(line, 5000, stdin);
		send(sockfd, line, strlen(line), 0);
		int n = recv(sockfd, line2, 5000, 0);
	
		if(n<0){
			printf("Receive error\n");
			return 1;
		}

		printf("Got from server %s\n", line2);
	}

	return 0;
}
