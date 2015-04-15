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
#include <map>

#define BYTES_TO_REC 256
#define WINDOW_SIZE 5
#define VALID_CHECKSUM 65535

using namespace std;

const int OVERHEAD = 5;
uint16_t window[WINDOW_SIZE];
uint16_t ALL_ONES = 65535;
uint16_t maxSequence = 65533;
typedef pair<char*,int> dataPair;
map<uint16_t, dataPair> dataMap;

uint16_t genChkSum(char * data, int size);
bool valChkSum(char * data, int size);
void write_to_file(ofstream *, map<uint16_t,dataPair>*);

int main(int argc, char **argv) {
	int windowCounter = 0;
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
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9010);
	string ipAddress = argv[2];
	serveraddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	  
	printf("Enter a file name: ");
	char line[5000];
	fgets(line,5000,stdin);
	char *pos;
	if ((pos=strchr(line, '\n')) != NULL)
    	*pos = '\0';

	sendto(sockfd,line,strlen(line),0,(struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in));

	char * path;
	path = (char *)malloc(strlen(line)+strlen("client/")+1);
    	path[0] = '\0';
    	strcat(path,line);
	
        char recvBuff[BYTES_TO_REC];

	socklen_t slen_server = sizeof(serveraddr);

	ofstream recFile;
	recFile.open(path);

	int bytes_received = recvfrom(sockfd, recvBuff, BYTES_TO_REC, 0, (struct sockaddr*)&serveraddr, &slen_server);

	uint16_t sequenceNumber;
	memcpy(&sequenceNumber, &recvBuff[2], 2);

	cerr << "RECEIVED PACKET#: " << sequenceNumber << endl;

	/*if (dataMap.count(sequenceNumber) > 0) {
		cerr << "PACKET " << sequenceNumber << " RECIEVED AGAIN. SENDING ACK." << endl;
		sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
	} else {
		cerr << "ADDING PACKET " << sequenceNumber << " TO MAP." << endl;
		
		char* storeValue;
		storeValue = (char*)malloc(sizeof(char)*(bytes_received - OVERHEAD));
		memcpy(storeValue, &recvBuff[OVERHEAD], bytes_received - OVERHEAD);
		dataMap[sequenceNumber] = make_pair(storeValue,bytes_received - OVERHEAD);
	}*/

	while(window[0] <= maxSequence){
		if(valChkSum(recvBuff,bytes_received)){
			cerr << "CHKSUM VALIDATED" << endl;
			if(recvBuff[4] == '1'){
				maxSequence = sequenceNumber;
			}
			int i = 0;
			if(window[i] == sequenceNumber){
				uint16_t sequenceNumberAfter = sequenceNumber + 1;

				while(dataMap.count(sequenceNumberAfter) > 0){
					sequenceNumberAfter++;
				}

				int sentSize = sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
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
		
				for(int i = 0; i < WINDOW_SIZE; i++){
					if(window[i] == ALL_ONES){
						window[i] = windowCounter;
						windowCounter++;
					}
				
				}
			}else{
				cerr << "PACKET IS OUT OF ORDER" << endl;
				for(int k = 1; k < WINDOW_SIZE; k++){	
					if(window[k] == sequenceNumber){
						window[k] = ALL_ONES;

						cerr << "SENDING ACK#: " << sequenceNumber << endl;
						sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
					}
				}
			}

			cerr << "RECEIVED PACKET#: " << sequenceNumber << endl;
			if (dataMap.count(sequenceNumber) > 0) {
				cerr << "PACKET " << sequenceNumber << " RECIEVED AGAIN. SENDING ACK." << endl;

				if(sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in)) < 0){
					cerr << "SEND ERROR" << endl;
				}
			}else{
				cerr << "ADDING PACKET " << sequenceNumber << " TO MAP." << endl;
			
				char* storeValue;
				storeValue = (char*)malloc(sizeof(char)*(bytes_received - OVERHEAD));
				memcpy(storeValue, &recvBuff[OVERHEAD], bytes_received - OVERHEAD);
				dataMap[sequenceNumber] = make_pair(storeValue,bytes_received - OVERHEAD);
			}

			if(window[0] >= maxSequence){
				break;
			}
		}
	
		memset(recvBuff, 0, sizeof(recvBuff));
		bytes_received = recvfrom(sockfd, recvBuff, BYTES_TO_REC, 0, (struct sockaddr*)&serveraddr, &slen_server);

		memcpy(&sequenceNumber, &recvBuff[2], 2);
		cerr << "Current slot: " << window[0] << " Max seq: " << maxSequence << endl;
	}

	printf("\nFile transferred\n");
	write_to_file(&recFile,&dataMap);

	recFile.close();
	close(sockfd);
  
  	return 0;
}

uint16_t genChkSum(char * data, int size){
	uint16_t chkSum = 0;
	
	data += 2;
	for(int i = 0; i < size + (OVERHEAD - 2); i++){
		//cerr << *data;
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

// write_to_file(FILE_POINTER,MAP_OF_DATA_POINTER)
void write_to_file(ofstream * f, map<uint16_t, dataPair> * m){
	typedef map<uint16_t,dataPair>::iterator it_type;
	for(it_type iterator = m->begin(); iterator != m->end();iterator++)
	{
		cerr << "Writing " << iterator->second.second << " bytes to file" << endl;
		f->write(iterator->second.first,iterator->second.second);
	}
}
