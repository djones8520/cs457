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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <mutex>

#define BYTES_TO_SEND 256
#define WINDOW_SIZE 5
#define BUFLEN 5000

void strip_newline(char* s);
void* receiveThread(void* arg);

uint16_t window[WINDOW_SIZE];

// Free window slot
uint16_t ALL_ONES = 65535;
// Acknowledged packet
uint16_t ONES_WITH_ONE_ZERO = 65534;

int sockfd = socket(AF_INET,SOCK_DGRAM,0);
struct sockaddr_in clientaddr;
socklen_t slen_client = sizeof(clientaddr);

using namespace std;

std::mutex windowLock;

int main(int argc, char **argv)
{	
	if(sockfd<0){
		printf("There was an error creating the socket\n");
		return 1;
	}
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9010);
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	char buf[BUFLEN]; 

	while(1){
		int total = 0;
		if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&clientaddr, &slen_client) < 0){  
			printf("Receive error. \n");
		}
		puts("Client connected");
		printf("Server received: %s\n",buf);
		FILE *fp = fopen(buf,"rb");
		if(fp == NULL){
			printf("Server: File read error\n");
			return 1;
		}
		
		// Set each item in window to all ones (ie free slot)
		for(int i = 0; i < WINDOW_SIZE; i++)
			window[i] = 65535;

		pthread_t thread;
		int status;
		if((status = pthread_create(&thread, NULL, receiveThread, &sockfd)) != 0){
		    cerr << "Error creating thread" << endl;
		}

		uint16_t currentSequence = 0;
		while(1){
			char readbuff[BYTES_TO_SEND - 3];
			char header[3]={'0','0','0'};
			int bytesRead = fread(readbuff,1,BYTES_TO_SEND - 3,fp);
			total+= bytesRead;
			
			memcpy(&header, &currentSequence, 2);
			cout << "Sequence #: " <<  currentSequence << endl;
			
			// Stay in this loop until there is a free spot in the window
			bool found = false;
			while(!found){
				windowLock.lock();
				for(int i = 0; i < WINDOW_SIZE; i++){
					//cout << "window " << i << " " << window[i] << endl;
					if(window[i] == ALL_ONES && !found){
						window[i] = currentSequence;
						found = true;
					}
				}

				windowLock.unlock();
			}
			
			// Increment sequence
			currentSequence++;
			
			
			//if(bytesRead < BYTES_TO_SEND - 3){
			if(bytesRead > 0){
				if(feof(fp)){
					header[2] = '1';
				}else if(ferror(fp)){
					puts("Server: Error while reading file");
				}
			}else if(bytesRead == 0){
				header[2] = '1';
			}

			char sendbuff[BYTES_TO_SEND];
			memcpy(sendbuff,header,3);
			memcpy(&sendbuff[3],readbuff,bytesRead);

			//printf("Server: BytesRead %d\n",bytesRead);
			//printf("Server: SendBuff Size... %d\n",strlen(sendbuff));
			//printf("Server: sent %s\n",&sendbuff[3]);
			
			sendto(sockfd,sendbuff,bytesRead + 3,0,
				(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in));
			
			if(bytesRead <= 0){
				puts("Server: Reached end of file");
				break;			
			}

		}
		printf("File sent.  Total Bytes... %d\n",total);
		fclose(fp);
	}
	close(sockfd);

	return 0;
}

void* receiveThread(void* arg){
	char buf[BYTES_TO_SEND];

	cout << "Receive thread created" << endl;
	
	while(1){
		if (recvfrom(sockfd, buf, BYTES_TO_SEND, 0, (struct sockaddr*)&clientaddr, &slen_client) < 0){  
				printf("Receive error. \n");
		}

		cout << "ACK" << endl;
		cout << "buf: " << buf << endl;
		
		uint16_t sequenceNumber;
		memcpy(&sequenceNumber, &buf[0], 2);
		uint8_t dataCheck;
		memcpy(&dataCheck, &buf[2], 1);

		cout << "dataCheck: " << dataCheck << endl;
		
		// If there is no more data, end the thread
		if(dataCheck != 0){
			cout << "Receive thread exit" << endl;
			
			break;
		}

		int i = 0;

		windowLock.lock();
		cout << "Thread sequence #: " << sequenceNumber << endl;
		if(window[i] == sequenceNumber){
			// window moves
			window[i] = ALL_ONES;

			for(i; i < WINDOW_SIZE; i++){
				bool found = false;
				
				for(int j = i+1; j < WINDOW_SIZE; j++){
					if(window[j] != ALL_ONES && window[j] != ONES_WITH_ONE_ZERO && !found){
						window[i] = window[j];
						window[j] = ALL_ONES;
						
						found = true;
					}
			
					if(j == (WINDOW_SIZE-1) && !found)
						window[i] = ALL_ONES;
				}
				
				// The last window slot won't be compared and must be set to ALL_ONES
				if(i == (WINDOW_SIZE - 1))
					window[i] = ALL_ONES;
			}
		}
		else{
			for(int k = 1; k < WINDOW_SIZE; k++){
				if(window[k] == sequenceNumber){
					window[k] = ONES_WITH_ONE_ZERO;
				}
			}
		}

		windowLock.unlock();

		memset(buf, 0, sizeof(buf));
	}
}

void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}
