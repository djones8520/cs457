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
#define OVERHEAD 3
#define VALID_CHECKSUM 65535
#define WINDOW_SIZE 5
#define BUFLEN 5000

using namespace std;

void strip_newline(char* s);
void* receiveThread(void* arg);
uint16_t genChkSum(char * data);
bool valChkSum(char * data);

uint16_t window[WINDOW_SIZE];
typedef pair<char*,int> dataPair;
map<uint16_t, dataPair> dataMap;


// Free window slot
uint16_t OPEN_SLOT = 65535;
// Acknowledged packet
uint16_t ACKNOWLEDGED = 65534;

int sockfd = socket(AF_INET,SOCK_DGRAM,0);
struct sockaddr_in clientaddr;
socklen_t slen_client = sizeof(clientaddr);

uint16_t maxSequence = 65533;
uint16_t ackSequence = 0;

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
			printf("File doesn't exist\n");

			char noFile[BYTES_TO_SEND];
			noFile[4] = '4';

			sendto(sockfd,noFile,BYTES_TO_SEND,0,
				(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in));
		}
		else{

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
				cout << "PREPING PACKET #: " <<  currentSequence << endl;			
				char readbuff[BYTES_TO_SEND - 3];
				char header[3]={'0','0','0'};
				int bytesRead = fread(readbuff,1,BYTES_TO_SEND - 3,fp);

				if(bytesRead <= 0){
					puts("Server: Reached end of file");
					break;
				}


				total+= bytesRead;

				memcpy(&header, &currentSequence, 2);
			
				/*
				// Stay in this loop until there is a free spot in the window
				bool found = false;
				while(!found){
					windowLock.lock();
					for(int i = 0; i < WINDOW_SIZE; i++){
						//cout << "window " << i << " " << window[i] << endl;
						if(window[i] == OPEN_SLOT && !found){
							window[i] = currentSequence;
							found = true;
						}
					}

					windowLock.unlock();
				}
				*/

				bool found = false;
				while(!found) {
					windowLock.lock();				
					for (int i = 0; i < WINDOW_SIZE; i++) {
						if (window[i] == OPEN_SLOT && !found) {
							window[i] = currentSequence;
							cout << "Send window: ";
							for(int m = 0; m < WINDOW_SIZE; m++){
								cout << window[m] << " ";
							}
							cout << endl;
							found = true;
						}
					}
					windowLock.unlock();
				}
			
			

				if(bytesRead <= BYTES_TO_SEND - 3 && bytesRead >= 0){
				//if(bytesRead > 0){
					if(feof(fp)){
						header[2] = '1';
						maxSequence = currentSequence;
					}else if(ferror(fp)){
						puts("Server: Error while reading file");
					}
				}else if(bytesRead == 0){
					//header[2] = '1';
					cout << "WHAAAAAATTTTT?????" << endl;
				}

				char sendbuff[BYTES_TO_SEND];
				memcpy(sendbuff,header,3);
				memcpy(&sendbuff[3],readbuff,bytesRead);

				dataMapLock.lock();

				uint16_t PacketNumber;
				memcpy(&PacketNumber, &sendbuff[0], 2);

				char* storeValue;
				storeValue = (char*)malloc(sizeof(char)*(bytesRead+3));
				memcpy(storeValue, &sendbuff, bytesRead+3);

				dataMap[PacketNumber] = make_pair(storeValue,bytesRead + 3);

				uint16_t dataMapSeq;
				memcpy(&dataMapSeq, dataMap[currentSequence].first, 2);
				cout << "Datamap Seq: " << dataMapSeq << " Current Seq: " << currentSequence << endl;
				dataMapLock.unlock();

				//bool chkSum = valChkSum(sendbuff);
				//cerr << "Checksum: " << chkSum << endl;

				//printf("Server: BytesRead %d\n",bytesRead);
				//printf("Server: SendBuff Size... %d\n",strlen(sendbuff));
				//printf("Server: sent %s\n",&sendbuff[3]);

				int sendSize = sendto(sockfd,sendbuff,bytesRead + 3,0,
					(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in));

				cout << "SENT PACKET #: " << currentSequence << endl;
			
				cout << "SIZE: " << sendSize << endl;
			

				// Increment sequence
				currentSequence++;

			}
			printf("File sent.  Total Bytes... %d\n",total);
			fclose(fp);
		}
	}
	close(sockfd);

	return 0;
}

void* receiveThread(void* arg){
	char buf[BYTES_TO_SEND];

	cout << "Receive thread created" << endl;

	fd_set select_fds;
	struct timeval timeout;
	int fd2 = *(int *)arg;
	FD_ZERO(&select_fds);
	FD_SET(fd2, &select_fds);

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	while(1){

		//FD_ZERO(&select_fds);
		//FD_SET(fd2,&select_fds);
		timeout.tv_sec = 1;
		if(select(fd2+1, &select_fds, NULL, NULL, &timeout) == 0){
			FD_ZERO(&select_fds);
			FD_SET(fd2,&select_fds);
			windowLock.lock();
			
			for(auto item : dataMap){
				if(sendto(fd2,item.second.first,item.second.second,0,(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in)) < 0){
						cerr << "Resend Error" << endl;
				}

				uint16_t resendReq;
				memcpy(&resendReq, item.second.first, 2);
				cerr << "Resending " << resendReq << endl;
			}
			/*
			for(int i = 0; i < WINDOW_SIZE; i++){
				if(window[i] != ACKNOWLEDGED && window[i] != OPEN_SLOT){
					if(sendto(fd2,dataMap[window[i]].first,dataMap[window[i]].second,0,(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in)) < 0){
						cerr << "Resend Error" << endl;
					}
					uint16_t resendReq;
					memcpy(&resendReq, dataMap[window[i]].first, 2);
					cerr << "Resending " << window[i] << " " << resendReq << endl;
				}

			}*/
			windowLock.unlock();
			//cerr << "The socket # is " << fd2 << endl;
			//timeout.tv_sec = 1;
			//cerr << "The socket is # " << fd2 << endl;
		}
		else{
			//cerr << "Got to ELSE" << endl;
			if (recvfrom(fd2, buf, BYTES_TO_SEND, 0, (struct sockaddr*)&clientaddr, &slen_client) < 0){
				printf("Receive error. \n");
			}

			//cout << "ACK" << endl;
			//cout << "buf: " << buf << endl;

			uint16_t RecvSeqNumber;
			memcpy(&RecvSeqNumber, &buf[0], 2);
			char dataCheck;
			memcpy(&dataCheck, &buf[2], 1);


			
			cout << "GOT ACK FOR: " << RecvSeqNumber << " dataCheck: " << dataCheck << endl;

			windowLock.lock();
			
			cout << "Resend Req: ";

			for(auto item : dataMap){				
				uint16_t seqResend;
				memcpy(&seqResend, item.second.first, 2);
				cout << seqResend << " ";
			}
			cout << endl;

			for (int j = 0; j < WINDOW_SIZE; j++) {
				if (window[j] == RecvSeqNumber) {
					window[j] = ACKNOWLEDGED;
					dataMapLock.lock();
					free(dataMap[RecvSeqNumber].first);
					dataMap.erase(RecvSeqNumber);
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

			cout << "WINDOW AFTER: ";
			for (int x = 0; x < WINDOW_SIZE; x++) {
				cout << window[x] << " ";
			}

			cout << endl;


			/*
			int i = 0;

			if(window[i] == RecvSeqNumber){
				
				dataMapLock.lock();
					dataMap.erase(sequenceNumber);
				dataMapLock.unlock();

				// window moves
				window[i] = OPEN_SLOT;

				for(i; i < WINDOW_SIZE; i++){
					bool found = false;

					for(int j = i+1; j < WINDOW_SIZE; j++){
						if(window[j] != OPEN_SLOT && window[j] != ACKNOWLEDGED && !found){
							window[i] = window[j];
							window[j] = OPEN_SLOT;

							found = true;
						}

						if(j == (WINDOW_SIZE-1) && !found)
							window[i] = OPEN_SLOT;
					}

					// The last window slot won't be compared and must be set to OPEN_SLOT
					if(i == (WINDOW_SIZE - 1)){
						window[i] = OPEN_SLOT;
					}
				}
			}
			else{
				for(int k = 1; k < WINDOW_SIZE; k++){
					if(window[k] == sequenceNumber){
						//window[k] = ACKNOWLEDGED;
						window[k] = OPEN_SLOT;
					}
					//cerr << "Window K " << window[k];
				}
				dataMapLock.lock();
					dataMap.erase(sequenceNumber);
				dataMapLock.unlock();
			}

			
			*/

			cout << "Max seq: " << maxSequence << " " << ackSequence << endl;
			// Once all packets have been acknowledged, exit
			if(ackSequence >= maxSequence){
				cout << "Receive thread exit" << endl;

				break;
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

uint16_t genChkSum(char * data){
	uint16_t chkSum = 0;
	
	data += OVERHEAD;
	int len = strlen(data);
	for(int i = 0; i < len; i++){
		cerr << *data;
		chkSum += *data;
		data++;
	}

	chkSum = ~chkSum;

	return chkSum;
}

bool valChkSum(char * data){
	uint16_t oldChkSum;
	memcpy(&oldChkSum,data,2);
	uint16_t newChkSum = 0;
	
	data += OVERHEAD;
	int len = strlen(data);
	for(int i = 0; i < len; i++){
		cerr << *data;
		newChkSum += *data;
		data++;
	}

	newChkSum |= oldChkSum;

	return newChkSum == VALID_CHECKSUM;
}
