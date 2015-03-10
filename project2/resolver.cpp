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
#include <time.h>

#define TYPE_A 1
#define CLASS_IN 1
#define BUFLEN 512
#define COMPRESSION 192

using namespace std;

struct dnsheader{
	uint16_t id;
	uint16_t flags;
	uint16_t qcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
};

struct dnsquery{
	string qname;
	uint16_t qtype;
	uint16_t qclass;
};

struct dnsresponse{
	string rname;
	uint16_t rtype;
	uint16_t rclass;
	uint32_t rttl;
	uint16_t rdlength;
	uint8_t* rdata;
};
struct dnspair{
	struct dnsresponse dr;
	time_t time_entered;
};
int get_header(dnsheader* h, char*buf, int* pos);
int get_query(dnsquery* q, char* buf, int* pos);
int get_response(dnsresponse *r, char* buf, int* pos);
bool check_cache(string name);
void unset_recursion_bit(void* q);
int valid_port(string s);
map<string, dnspair> cache;\
void CatchAlarm(int);

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
	signal (SIGALRM, CatchAlarm);

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
	char recbuf[BUFLEN];
	socklen_t socketLength = sizeof(clientaddr);
	socklen_t rootLength = sizeof(rootaddr);

	while (1){
		if (recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&clientaddr, &socketLength) < 0){
			cerr << "Receive error in IF" << endl;
			return 1;
		}

		dnsheader h;
		dnsquery q;
		int pos = 0;

		if (get_header(&h, buf, &pos) < 0){
			cerr << "Unable to get header info" << endl;
		}

		if (get_query(&q, buf, &pos) < 0){
			cerr << "Unable to get query info" << endl;
		}

		if (check_cache(q.qname)){
			sendto(sockfd, &cache[q.qname].dr, sizeof(cache[q.qname].dr), 0, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_in));
			// cache[q.qname].dr is the dnsresponse to send back
			//return IP<F7>
		}
		else{
			bool found = false;
			
			while(!found){
				unset_recursion_bit(&q);
				sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&rootaddr,sizeof(struct sockaddr_in));
				alarm(2);
				if (recvfrom(sockfd, recbuf, BUFLEN, 0, (struct sockaddr*)&rootaddr, &rootLength) < 0){
					perror("Receive error");
					return 0;
				}
				alarm(0);
				
				dnsheader rh;
				dnsquery rq;
				pos = 0;
				
				if (get_header(&rh, recbuf, &pos) < 0){
					cerr << "Unable to get header info" << endl;
				}

				if (get_query(&rq, recbuf, &pos) < 0){
					cerr << "Unable to get query info" << endl;
				}

				int response_num = rh.ancount + rh.nscount + rh.arcount;
				dnsresponse r[response_num];

				for(int i = 0; i < response_num; i++){
					if (get_response(&(r[i]), recbuf, &pos) < 0){
						cerr << "Unable to get response info" << endl;
					}
				}
			
				// INSERT CHECK IF AN ANSWER WAS FOUND
				if(q.qname.compare("check")){
					// INSERT RESPOND TO CLIENT AND SET TO CACHE
					// sendto(sockfd, DATA TO SEND, sizeof(DATA TO SEND), 0, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_in));
					
					if(cache.size() < cache.max_size()){
						//struct dnspair tempPair;
						//temp_pair.dnsresponse = 
						//temp_pair->time_entered = time(NULL);
						// cache[q.name] = tempPair;
					}
					else{
						found = true;
					}
				}
			}
		}
		memset(buf, 0, sizeof(buf));
	}
	return 0;
}

int get_header(dnsheader* h, char* buf, int* pos){
	int start = *pos;
	memcpy(h, buf, 12);
	*pos = 12;
	cout << "DNS Header" << endl;
	cout << "------------------------------------" << endl;
	cout << "ID: 0x" << hex << ntohs(h->id) << endl;
	cout << "Flags: 0x" << hex << ntohs(h->flags) << endl;
	cout << "QCOUNT: " << ntohs(h->qcount) << endl;
	cout << "ANCOUNT: " << ntohs(h->ancount) << endl;
	cout << "NSCOUNT: " << ntohs(h->nscount) << endl;
	cout << "ARCOUNT: " << ntohs(h->arcount) << endl;

	for(int i = start; i < *pos; i++){
 		printf("%02X ",buf[i]);
 	}

 	cout << endl << endl;

	return 0;
}

int get_query(dnsquery* q, char* buf, int* pos){
	int start = *pos;
	string name;
	short length;
	char tempchar;
	memcpy(&length, &buf[*pos], 1);
	(*pos)++;
	while (length != 0){
		for (int i = 0; i < length; i++){
			memcpy(&tempchar, &buf[(*pos)++], 1);
			name += tempchar;
		}
		name += ".";
		memcpy(&length, &buf[(*pos)++], 1);
	}
	q->qname = name;
	memcpy(&(q->qtype),&buf[*pos],2);
	*pos += 2;
	memcpy(&(q->qclass),&buf[*pos],2);
	*pos += 2;

	cout << "QNAME: " << q->qname << endl;
	cout << "QTYPE: " << ntohs(q->qtype) << endl;
	cout << "QCLASS: " << ntohs(q->qclass) << endl;
	//pos += 4;
	
	for(int i = start; i < *pos; i++){
 		printf("%02X ",buf[i]);
 	}
 	cout << endl << endl;
	
	//just returning 0 to avoid warning
	return 0;
}

int get_response(dnsresponse* r, char* buf, int* pos){
	int start = *pos;
	char tempchar;
	string name;
	uint16_t length;
	uint16_t ofs = 0; //offset
	int cmpcnt = 0; //compression count
	cerr << "1" << endl;
	memcpy(&length, &buf[(*pos)++], 1);
	cerr << "2" << endl;
	while(length != 0){
		if(length >= COMPRESSION){
			cerr << "3" << endl;
			cmpcnt++;
			if(cmpcnt > 1){
				memcpy(&ofs, &buf[ofs-1], 2);
				ofs << 2; //get rid of first two bits
				ofs >> 2;
				memcpy(&length, &buf[ofs++], 1);
			}else{
				memcpy(&ofs, &buf[(*pos)-1], 2);
				(*pos)++;
				ofs << 2;
				ofs >> 2;
				memcpy(&length, &buf[ofs++], 1);
			}
		}else if(ofs != 0){
			cerr << "4" << endl;
			for(int i = 0; i < length; i++){
				memcpy(&tempchar, &buf[ofs++], 1);
	          	name += tempchar;
			}
			name += ".";
	        memcpy(&length, &buf[ofs++], 1);
		}else{
			cerr << "5" << endl;
			for(int i = 0; i < length; i++){
				memcpy(&tempchar, &buf[(*pos)++], 1);
	          	name += tempchar;
			}
			name += ".";
	        memcpy(&length, &buf[(*pos)++], 1);
		}
	}
	cerr << "Resolved name" << endl;
	r->rname = name;

	memcpy(&(r->rtype),&buf[*pos],2);
	*pos += 2;
	memcpy(&(r->rclass),&buf[*pos],2);
	*pos += 2;
	memcpy(&(r->rttl),&buf[*pos],4);
	*pos += 4;
	memcpy(&(r->rdlength),&buf[*pos],2);
	*pos += 2;
	memcpy(&(r->rdata),&buf[*pos],r->rdlength);
	*pos += r->rdlength;

	cout << "RNAME: " << r->rname << endl;
	cout << "RTYPE: " << ntohs(r->rtype) << endl;
	cout << "RCLASS: " << ntohs(r->rclass) << endl;
	cout << "RTTL: " << ntohs(r->rttl) << endl;
	cout << "RDLENGTH: " << ntohs(r->rdlength) << endl;
	//cout << "RDATA: " << ntohs(r->rdata) << endl;

	for(int i = start; i < *pos; i++){
 		printf("%02X ",buf[i]);
 	}

 	cout << endl << endl;

	return 0;
}

// Check if name is in cache
bool check_cache(string name){
	dnspair myPair = cache[name];
	if(cache.count(name) != 0){
		if(time(NULL) > (myPair.dr.rttl + myPair.time_entered)){
			cache.erase(name);
			return false;			
		}
		return true;
	}
	else
		return false;	
}

void unset_recursion_bit(void* q){
	/*uint16_t temp = 65279; //1111111011111111 the 0 is the RD bit
	dnsquery* query = (dnsquery*)q;

	query->flags &= temp;*/
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

void CatchAlarm(int ignored){
	cout << "Server took too long to respond\nQuitting...\n";
	exit(1);
}
