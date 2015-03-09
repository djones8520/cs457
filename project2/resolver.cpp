/****************************************************************
 * Project 2: DNS Resolver
 * Due 3/11/2015
 *
 * Michael Kinkema
 * Chase Pietrangelo
 * Danny Selgo
 * Daniel Jones
 ****************************************************************/

//note that this code relies on c++11 features, and thus must be
//compiled with the -std=c++11 flag when using g++

//need to go through and remove irrelevant libraries
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string.h>
#include <random>
#include <chrono>
#include <cstring>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <map>
#include <ctype.h>

#define TYPE_A 1
#define CLASS_IN 1
#define BUFLEN 512

using namespace std;

struct dnsquery{
	uint16_t id;
	uint16_t flags;
	uint16_t qcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
	string qname;
	uint16_t qtype;
	uint16_t qclass;
};

struct dnsresponse{
	uint16_t id;
	uint16_t flags;
	uint16_t qcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
	uint16_t type;
	uint16_t dns_class;
	uint32_t ttl;
	uint16_t rdlength;
	uint8_t* rdata;
	string name;
};

int get_query(dnsquery* q, char* buf);
bool check_cache(string name);
void unset_recursion_bit(void* q);
int valid_port(string s);
map<string, dnsresponse> cache;

/*
 * Link to RFC 1034: https://www.ietf.org/rfc/rfc1034.txt
 * Link to RFC 1035: https://www.ietf.org/rfc/rfc1035.txt section 7 is extremely relevant
 *
 * Steps to take:
 * 1. Open UDP socket with client and accept DNS query
 *    -Possibly create seperate threads to handle queries to leave socket open for more connections
 *    -Figure out how to "maintain connection" (didn't do in Lab4 and I don't understand how)
 * 2. Parse query and put into a dnsquery struct
 * 3. Check if name is cached
 *    -If so, send dnsresponse to client
 * 4. Unset the RecursionDesired (RD) bit in the query flags
 *    -This will prevent other name servers from forwarding the query further
 * 5. Open UDP socket to a name server from: http://www.internic.net/domain/named.root
 * 6. Send query to name server
 * 7. When we receive a successful response back, cache the response and forward response packet to client
 * 8. Keep track of TTL in cache
 *
 * dig @127.0.0.1 -p 9010 gvsu.edu
 */




int main(int argc, char** argv){

	int port = 9010;
	string logfile;

	//only need -p, but maybe we could use the others?
	if (argc > 1){
		for (int i = 1; i < argc; i++){
			if (strcmp(argv[i], "-p") == 0){
				i++;
			
				if(!valid_port(argv[i]))
					return 1;
			}
			else{
				cerr << "Invalid option. Only valid option is -p\n";
				return 1;
			}
		}
	}
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0){
		printf("Problem creating socket\n");
		return 1;
	}

	struct sockaddr_in serveraddr, clientaddr, rootaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	//later might want to add the rest of the name servers in case we don't receive a response back
	rootaddr.sin_family = AF_INET;
	rootaddr.sin_port = htons(53); //apparently this is the port DNS servers use?
	rootaddr.sin_addr.s_addr = inet_addr("192.203.230.10"); //A.ROOT-SERVERS.NET. 

	bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));

	char buf[BUFLEN];
	char recBuf[BUFLEN];
	socklen_t socketLength = sizeof(clientaddr);
	socklen_t rootLength = sizeof(rootaddr);

	while (1){
		if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&clientaddr, &socketLength) < 0){
			cerr << "Receive error in IF" << endl;
			return 1;
		}

		dnsquery q;

		if (get_query(&q, buf) < 0){
			cerr << "Unable to get query info" << endl;
		}

		if (check_cache(q.qname)){
			sendto(sockfd, &cache[q.qname], sizeof(cache[q.qname]), 0, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_in));
			// cache[q.qname] is the dnsresponse to send back
			//return IP
		}
		else{
			bool found = false;
			
			while(!found){
				unset_recursion_bit(&q, &buf);
				sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&rootaddr,sizeof(struct sockaddr_in));
				if (recvfrom(sockfd, recBuf, BUFLEN, 0, (struct sockaddr*)&rootaddr, &rootLength) < 0){
					perror("Receive error");
					return 0;
				}
				
				if (get_query(&q, recBuf) < 0){
					cerr << "Unable to get query info" << endl;
				}
			
				// INSERT CHECK IF AN ANSWER WAS FOUND
				if(q.qname == "check"){
					// INSERT RESPOND TO CLIENT AND SET TO CACHE
					// sendto(sockfd, DATA TO SEND, sizeof(DATA TO SEND), 0, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_in));
					
					// cache[q.name] = data;
					found = true;
				}
			}
			//code here to analyze response and determine if we should forward the 
			//answer to another nameserver or back to the client
		}
		

		memset(buf, 0, sizeof(buf));
	}
	return 0;
}

int get_query(dnsquery* q, char* buf){
	memcpy(q, buf, 12);
	int pos = 12;
	string name;
	short length;
	char tempchar;
	memcpy(&length, &buf[pos], 1);
	pos++;
	while (length != 0){
		for (int i = 0; i < length; i++){
			tempchar = (char)(buf[pos++]);
			name += tempchar;
		}

		name += ".";
		memcpy(&length, &buf[pos], 1);
		pos++;
	}
	q->qname = name;
	memcpy(&(q->qtype),&buf[pos],2);
	pos += 2;
	memcpy(&(q->qclass),&buf[pos],2);

	cout << "Request Header" << endl;
	cout << "------------------------------------" << endl;
	cout << "ID: 0x" << hex << ntohs(q->id) << endl;
	cout << "Flags: 0x" << hex << ntohs(q->flags) << endl;
	cout << "QCOUNT: " << ntohs(q->qcount) << endl;
	cout << "ANCOUNT: " << ntohs(q->ancount) << endl;
	cout << "NSCOUNT: " << ntohs(q->nscount) << endl;
	cout << "ARCOUNT: " << ntohs(q->arcount) << endl;
	cout << "QNAME: " << q->qname << endl;
	cout << "QTYPE: " << ntohs(q->qtype) << endl;
	cout << "QCLASS: " << ntohs(q->qclass) << endl << endl;
	pos += 4;
	for(int i = 0; i < pos; i++){
		printf("%02X ",buf[i]);
	}
	cout << endl << endl;

	//just returning 0 to avoid warning
	return 0;
}

// Check if name is in cache
bool check_cache(string name){
	if(cache.count(name) != 0)
		return true;
	else
		return false;	
}

void unset_recursion_bit(void* q, char* buf){	
	buf[2] &= 254; // 11111110
	
	uint16_t temp = 65279; //1111111011111111 the 0 is the RD bit
	dnsquery* query = (dnsquery*)q;

	query->flags &= temp;
}

int valid_port(string s)
{
	int port;
	for(unsigned int j = 0; j < s.length();j++)
	{
		if(!isdigit(s.at(j))){
			cerr << "Invalid port number: please only enter numeric characters\n";
			return 0;	
		}
		port = atoi(&s[0]);
		if (port < 0 || port > 61000){
			cerr << "Invalid port: port number must be between 0 and 61,000\n";
			return 0;
		}
	}
	return 1;
}
