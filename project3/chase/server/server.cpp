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
//map<int, dataPair> dataMap;
map<uint16_t, dataPair> dataMap;

// Free window slot
uint16_t OPEN_SLOT = 65535;
// Acknowledged packet
uint16_t ACKNOWLEDGED = 65534;

int sockfd = socket(AF_INET,SOCK_DGRAM,0);
struct sockaddr_in clientaddr;
socklen_t slen_client = sizeof(clientaddr);

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
			
			
			
			
			if(bytesRead <= BYTES_TO_SEND - 3 && bytesRead >= 0){
			//if(bytesRead > 0){
				if(feof(fp)){
					header[2] = '1';
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

			cerr << "DATAMAP: " << endl;
			for(std::map<int,dataPair>::iterator it=dataMap.begin(); it!=dataMap.end(); ++it){
				cerr << "Sequence: " << it->first << endl;
			}


			bool found = false;
			while(!found) {
				windowLock.lock();				
				for (int i = 0; i < WINDOW_SIZE; i++) {
					if (window[i] == OPEN_SLOT && !found) {
						window[i] = currentSequence;

						dataMapLock.lock();

						uint16_t PacketNumber;
						memcpy(&PacketNumber, &sendbuff[0], 2);
						cout << "EXTRACTED/ADDED TO MAP: " << PacketNumber << endl;
						dataMap[PacketNumber] = make_pair(sendbuff,bytesRead + 3);

						//cout << "ADDING TO MAP: " << currentSequence << endl;
						
						
						dataMapLock.unlock();

						found = true;
						cout << "ADDED TO WINDOW: " << endl;
							for (int x = 0; x < WINDOW_SIZE; x++) {
							cout << "WINDOW[" << x << "]: " << window[x] << endl;
						}
					}
				}
				windowLock.unlock();
			}



			

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
	close(sockfd);

	return 0;
}

void* receiveThread(void* arg){
	char buf[BYTES_TO_SEND];

	cerr << "Receive thread created" << endl;

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
			for(int i = 0; i < WINDOW_SIZE; i++){
				if(window[i] != ACKNOWLEDGED && window[i] != OPEN_SLOT){
					if(sendto(fd2,dataMap[window[i]].first,dataMap[window[i]].second,0,(struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in)) < 0){
						cerr << "Resend Error" << endl;
					}
					cerr << "Resending Window#: " << window[i] << endl;
					int tmpSeq;
					memcpy(&tmpSeq,dataMap[window[i]].first,2);
					cerr << "Resending Packet#: " << tmpSeq << endl;
				}

			}
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


			
			cout << "GOT ACK FOR: " << RecvSeqNumber << endl;
			cout << "dataCheck: " << dataCheck << endl;

			// If there is no more data, end the thread
			if(dataCheck != '0'){
				cout << "Receive thread exit" << endl;

				break;
			}

			cout << "WINDOW BEFORE: " << endl;
			for (int x = 0; x < WINDOW_SIZE; x++) {
				cout << "WINDOW[" << x << "]: " << window[x] << endl;
			}

			windowLock.lock();

			for (int j = 0; j < WINDOW_SIZE; j++) {
				if (window[j] == RecvSeqNumber) {
					window[j] = ACKNOWLEDGED;
					dataMapLock.lock();
					dataMap.erase(RecvSeqNumber);
					dataMapLock.unlock();
				}
			}

			while (window[0] == ACKNOWLEDGED) {
				for (int k = 0; k < WINDOW_SIZE-1; k++) {
					window[k] = window[k+1];
				}
				window[WINDOW_SIZE-1] = OPEN_SLOT;
			}

			windowLock.unlock();

			cout << "WINDOW AFTER: " << endl;
			for (int x = 0; x < WINDOW_SIZE; x++) {
				cout << "WINDOW[" << x << "]: " << window[x] << endl;
			}



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
