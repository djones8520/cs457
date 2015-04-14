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

using namespace std;

#define BYTES_TO_REC 256
#define OVERHEAD 5
#define WINDOW_SIZE 5
#define VALID_CHECKSUM 65535
uint16_t window[WINDOW_SIZE];
// Free window slot
uint16_t ALL_ONES = 65535;
uint16_t maxSequence = 65533;
map<uint16_t, char[BYTES_TO_REC-3]> dataToWrite;

uint16_t genChkSum(char * data);
bool valChkSum(char * data);

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
	string ipAddress = argv[2];

	serveraddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	//serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	  
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

	uint16_t sequenceNumber;
	memcpy(&sequenceNumber, &recvBuff[0], 2);



	// CHECKS TO MAKE SURE PACKET IS NOT ALREADY IN THE MAP
	if (dataToWrite.count(sequenceNumber) > 0) {
		// SENDS BACK ACK
		cerr << "PACKET " << sequenceNumber << " RECIEVED AGAIN. SENDING ACK." << endl;
		sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
	} else {
		// ADD PACKET TO MAP ONCE RECEIVED
		cerr << "ADDING PACKET " << sequenceNumber << " TO MAP." << endl;
		memcpy(&dataToWrite[sequenceNumber], &recvBuff[3], BYTES_TO_REC-3);
	}


	
	
	cout << "Current slot: " << window[0] << " Max seq: " << maxSequence << endl;
	while(window[0] <= maxSequence){
			cout << "client dataCheck: " << recvBuff[2] << endl;
			if(recvBuff[2] == '1'){
				maxSequence = sequenceNumber;
			}

			int i = 0;
			cout << "Current seq: " << window[i] << " " << "Seq rec: " << sequenceNumber << endl;
			if(window[i] == sequenceNumber){
				

				// PACKET IN WINDOW (window moves)			
				recFile.write(&recvBuff[3],bytes_received-3);

				// Write items in out of order packets in dataToWrite
				uint16_t sequenceNumberAfter = sequenceNumber++;
				while(dataToWrite.count(sequenceNumberAfter) > 0){
					recFile.write(dataToWrite[sequenceNumberAfter],bytes_received-3);
					sequenceNumberAfter++;
				}


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
			
				// IF WITHIN WINDOW
				for(int k = 1; k < WINDOW_SIZE; k++){	
					if(window[k] == sequenceNumber){	

						// PACKET IS IN WINDOW					
						window[k] = ALL_ONES;

						// COMMENTED OUT BECUASE IT SHOULD BE ALREADY IN THE MAP
						//memcpy(&dataToWrite[sequenceNumber], &recvBuff[3], BYTES_TO_REC-3);

						// Send acknowledgement
						sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
					}
				}
			}
		
		
		//printf("Got from the server \n%s\n", &recvBuff[3]);

		
		if (dataToWrite.count(sequenceNumber) > 0) {
			// SENDS BACK ACK
			//cerr << "PACKET " << sequenceNumber << " RECIEVED AGAIN. SENDING ACK." << endl;
			if(sendto(sockfd, recvBuff, bytes_received, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in)) < 0){
				cerr << "SEND ERROR" << endl;
			}
		} else {
			// ADD PACKET TO MAP ONCE RECEIVED
			//cerr << "ADDING PACKET " << sequenceNumber << " TO MAP." << endl;
			memcpy(&dataToWrite[sequenceNumber], &recvBuff[3], BYTES_TO_REC-3);
		}

		memcpy(&sequenceNumber, &recvBuff[0], 2);
		cout << "Current slot: " << window[0] << " Max seq: " << maxSequence << endl;
		
		if(window[0] >= maxSequence){
			cout << "Exit Current slot: " << window[0] << " Max seq: " << maxSequence << endl;
			break;
		}
	
		memset(recvBuff, 0, sizeof(recvBuff));
		bytes_received = recvfrom(sockfd, recvBuff, BYTES_TO_REC, 0, (struct sockaddr*)&serveraddr, &slen_server);


		cout << "-----------------" << endl;
		//cout << "WINDOW[0]:    " << window[0] << endl;
		cout << "window: ";
		for(int j = 0; j < 5; j++) {
			cout << window[j] << " ";
		}
		cout << endl;

		cout << "MAX SEQUENCE: " << maxSequence << endl;
		cout << "-----------------" << endl;
	}

	printf("\nFile transferred\n");

	recFile.close();
	close(sockfd);
  
  	return 0;
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
