/**********************************************************************************
* Project 3
* Due 4/1/2015
*
* Michael Kinkema
* Danny Selgo
* Daniel Jones
* Chase pietrangelo
*
**********************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define BYTES_TO_REC 256
#define WINDOW_SIZE 5

uint16_t window[WINDOW_SIZE];
// Free window slot
uint16_t ALL_ONES = 65535;

using namespace std;

int main(int argc, char **argv) {
	// Next sequence number to be put into window when it moves
	int windowCounter = 0;
	
	// Intialize window
	for(int i = 0; i < WINDOW_SIZE; i++){
		window[i] = i;
		
		windowCounter++;
	}
	
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (sockfd < 0) {
		printf("There was a problem creating the socket\n");
		return 1;
	}
	  
	char serverPort[5000];
	char serverIP[5000];
	/*  
	printf("Enter server port number\n");
	fgets(serverPort,5000,stdin);
	  
	printf("Enter server IP address\n");
	fgets(serverIP,5000,stdin);
	*/  
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET; //9010
	// atoi(serverPort)
	serveraddr.sin_port = htons(9010);
	// serverIP
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	  
	printf("Enter a file name: ");
	char line[5000];
	fgets(line,5000,stdin);
	char *pos;
	if ((pos=strchr(line, '\n')) != NULL)
    	*pos = '\0';

	sendto(sockfd,line,strlen(line),0,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in));

	char * path;
	path = (char *)malloc(strlen(line)+strlen("client/")+1);
    	path[0] = '\0';   // ensures the memory is an empty string
    	//strcat(path,"client/");
    	strcat(path,line);
	
        char recvBuff[BYTES_TO_REC];

	socklen_t slen_server = sizeof(serveraddr);

	ofstream recFile;
	recFile.open(path);

	int bytes_received = recvfrom(sockfd, recvBuff, BYTES_TO_REC, 0, (struct sockaddr*)&serveraddr, &slen_server);
	cout << "client dataCheck: " << recvBuff[2] << endl;
	//printf("rec buff: %c\n",recvBuff[2]);
	//cerr << "rec buff: " << recvBuff[2] << endl;
	while(recvBuff[2] != '1'){
		// PUT IN AREA WHERE PACKET IS RECEIVED
		uint16_t sequenceNumber;
		memcpy(&sequenceNumber, &recvBuff[0], 2);

		int i = 0;
		if(window[i] == sequenceNumber){
			// PACKET IN WINDOW (window moves)
			
			recFile.write(&recvBuff[3],BYTES_TO_REC-3);

			// Send acknowledgement
			int sentSize = sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
			cout << "bytes_rec: " << bytes_received << " bytes_sent: " << sentSize << endl;

			window[i] = ALL_ONES;
			
			for(i; i < WINDOW_SIZE; i++){
				bool found = false;
				for(int j = i+1; j < WINDOW_SIZE; j++){
					if(window[j] != ALL_ONES && !found){
						window[i] = window[j];
						window[j] = ALL_ONES;
						
						found = true;
					}
				}
			}
			
			cout << "window counter: " << windowCounter << endl;
			// items in the window have moved to the front, now add to free spot(s)
			for(int i = 0; i < WINDOW_SIZE; i++){
				if(window[i] == ALL_ONES){
					window[i] = windowCounter;
					windowCounter++;
					
					//cout << "client window updated" << endl;
				}
				
				cout << window[i];
			}
			
			cout << endl << "end window move" << endl;
		}
		else{
			// IN PART ONE, IT SHOULD NEVER GET HERE
			cout << "PACKET IS OUT OF ORDER" << endl;
			
			for(int k = 1; k < WINDOW_SIZE; k++){	
				if(window[k] == sequenceNumber){
					// PACKET IS IN WINDOW					
					// Write to file
					recFile.write(&recvBuff[3],BYTES_TO_REC-3);
					// Send acknowledgement
					sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
				}
			}
		}
		
		//printf("Got from the server \n%s\n", &recvBuff[3]);
		memset(recvBuff, 0, sizeof(recvBuff));
		bytes_received = recvfrom(sockfd, recvBuff, BYTES_TO_REC, 0, (struct sockaddr*)&serveraddr, &slen_server);
	}
	
	// Send final ACK
	sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
	
	recFile.write(&recvBuff[3],bytes_received - 3);
	//printf("Got from the server %s\n", recvBuff);
	printf("\nFile transferred\n");

	recFile.close();
	close(sockfd);
  
  	return 0;
}
