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
#include <map>
#include <utility>

#define BYTES_TO_SEND 256
#define VALID_CHECKSUM 65535
#define WINDOW_SIZE 5
#define BUFLEN 5000

using namespace std;

void strip_newline(char* s);
void* receiveThread(void* arg);
uint16_t genChkSum(char * data, int size);
bool valChkSum(char * data, int size);

uint16_t window[WINDOW_SIZE];
typedef pair<char*,int> dataPair;
map<uint16_t, dataPair> dataMap;

const int OVERHEAD = 5;
uint16_t OPEN_SLOT = 65535;
uint16_t ACKNOWLEDGED = 65534;

int sockfd = socket(AF_INET,SOCK_DGRAM,0);
struct sockaddr_in clientaddr;
socklen_t slen_client = sizeof(clientaddr);

uint16_t maxSequence = 65533;
uint16_t ackSequence = 0;

int MAX_RESEND = 10;

std::mutex windowLock;
std::mutex dataMapLock;

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

		for(int i = 0; i < WINDOW_SIZE; i++){
			window[i] = 65535;
		}

		pthread_t thread;
		int status;
		if((status = pthread_create(&thread, NULL, receiveThread, &sockfd)) != 0){
		    cerr << "Error creating thread" << endl;
		}

		uint16_t currentSequence = 0;
		while(maxSequence > currentSequence){
			cout << "PREPING PACKET #: " <<  currentSequence << endl;			
			char readbuff[BYTES_TO_SEND - OVERHEAD];
			char header[OVERHEAD]={'0','0','0','0','0'};
			int bytesRead = fread(readbuff,1,BYTES_TO_SEND - OVERHEAD,fp);

			if(bytesRead <= 0){
				puts("Server: Reached end of file");
				break;
			}

			total+= bytesRead;
			memcpy(&header[2], &currentSequence, 2);
			
			if(bytesRead <= BYTES_TO_SEND - OVERHEAD && bytesRead >= 0){
				if(feof(fp)){
					header[4] = '1';
					maxSequence = currentSequence;
				}else if(ferror(fp)){
					puts("Server: Error while reading file");
				}
			}else if(bytesRead == 0){
				cout << "WHAAAAAATTTTT?????" << endl;
			}

			char sendbuff[BYTES_TO_SEND];
			memcpy(sendbuff,header,OVERHEAD);
			memcpy(&sendbuff[OVERHEAD],readbuff,bytesRead);

			bool found = false;
			while(!found) {
				windowLock.lock();				
				for (int i = 0; i < WINDOW_SIZE; i++) {
					if (window[i] == OPEN_SLOT && !found) {
						window[i] = currentSequence;
						dataMapLock.lock();

						uint16_t packetNumber;
						memcpy(&packetNumber, &sendbuff[2], 2);

						char* storeValue;
						storeValue = (char*)malloc(sizeof(char)*(bytesRead + OVERHEAD));
						memcpy(storeValue, &sendbuff, bytesRead + OVERHEAD);

						dataMap[packetNumber] = make_pair(storeValue,bytesRead + OVERHEAD);

						dataMapLock.unlock();

						found = true;
					}
				}
				windowLock.unlock();
			}

			//bool chkSum = valChkSum(sendbuff);
			//cerr << "Checksum: " << chkSum << endl;

			//printf("Server: BytesRead %d\n",bytesRead);
			//printf("Server: SendBuff Size... %d\n",strlen(sendbuff));
			//printf("Server: sent %s\n",&sendbuff[OVERHEAD]);

			uint16_t chkSum = genChkSum(sendbuff,bytesRead);
			memcpy(&sendbuff[0],&chkSum,2);
			cerr << "Generated Checksum: " << chkSum << endl;
			cerr << "Checksum in Packet: " << sendbuff[0] << sendbuff[1] << endl;

			int sendSize = sendto(sockfd,sendbuff,bytesRead + OVERHEAD,0,
				(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in));

			cerr << "SENT PACKET #: " << currentSequence << endl;
			cerr << "SIZE: " << sendSize << endl;
			currentSequence++;
		}
		printf("File sent.  Total Bytes... %d\n",total);
		fclose(fp);
	}
	close(sockfd);

	return 0;
}

void* receiveThread(void* arg){
	char buf[BYTES_TO_SEND];
	int resendCount = 0;

	fd_set select_fds;
	struct timeval timeout;
	int fd2 = *(int *)arg;
	FD_ZERO(&select_fds);
	FD_SET(fd2, &select_fds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 250000;

	while(1){
		timeout.tv_sec = 1;
		if(select(fd2+1, &select_fds, NULL, NULL, &timeout) == 0){
			if(resendCount >= MAX_RESEND){
				cerr << "Max resend reached.  Exiting Receive thread..." << endl;
				return 0;
			}

			resendCount++;
			//cerr << "Resending Window#: " << window[i] << endl;

			FD_ZERO(&select_fds);
			FD_SET(fd2,&select_fds);
			windowLock.lock();

			for(int i = 0; i < WINDOW_SIZE; i++){
				if(window[i] != ACKNOWLEDGED && window[i] != OPEN_SLOT){
					if(sendto(fd2,dataMap[window[i]].first,dataMap[window[i]].second,0,(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in)) < 0){
						cerr << "Resend Error" << endl;
					}
				}

			}
			windowLock.unlock();
		}else{
			int bytesReceived;
			if(bytesReceived = recvfrom(fd2, buf, BYTES_TO_SEND, 0, (struct sockaddr*)&clientaddr, &slen_client) < 0){
				printf("Receive error. \n");
			}

			if(valChkSum(buf,bytesReceived)){
				cerr << "CHKSUM VALIDATED" << endl;
				uint16_t recvSeqNumber;
				memcpy(&recvSeqNumber, &buf[2], 2);
				char dataCheck;
				memcpy(&dataCheck, &buf[4], 1);
				resendCount = 0;

				cerr << "GOT ACK FOR: " << recvSeqNumber << endl;

				windowLock.lock();

				for (int j = 0; j < WINDOW_SIZE; j++) {
					if (window[j] == recvSeqNumber) {
						window[j] = ACKNOWLEDGED;
						dataMapLock.lock();
						free(dataMap[recvSeqNumber].first);
						dataMap.erase(recvSeqNumber);
						dataMapLock.unlock();

						ackSequence++;
					}
				}

				while (window[0] == ACKNOWLEDGED) {
					for (int k = 0; k < WINDOW_SIZE-1; k++) {
						window[k] = window[k+1];
					}
					window[WINDOW_SIZE-1] = OPEN_SLOT;
				}

				windowLock.unlock();

				cerr << "Max seq: " << maxSequence << " " << ackSequence << endl;
				// Once all packets have been acknowledged, exit
				if(ackSequence > maxSequence){
					cout << "Receive thread exit" << endl;
					return 0;
				}

			}

			memset(buf, 0, sizeof(buf));
			FD_ZERO(&select_fds);
			FD_SET(fd2,&select_fds);
		}
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

uint16_t genChkSum(char * data, int size){
	uint16_t chkSum = 0;
	
	data += 2;
	for(int i = 0; i < size + (OVERHEAD - 2); i++){
		cerr << *data;
		chkSum += *data;
		data++;
	}

	chkSum = ~chkSum;
	return chkSum;
}

bool valChkSum(char * data, int size){
	uint16_t oldChkSum;
	memcpy(&oldChkSum,data,2);
	uint16_t newChkSum = 0;
	
	data += 2;
	for(int i = 0; i < size + (OVERHEAD - 2); i++){
		cerr << *data;
		newChkSum += *data;
		data++;
	}

	newChkSum |= oldChkSum;
	return newChkSum == VALID_CHECKSUM;
}
