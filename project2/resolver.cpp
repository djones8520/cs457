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
#include <stdlib.h>
#include <random>
#include <chrono>
#include <cstring>
#include <stack>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
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
	int32_t rttl;
	uint16_t rdlength;
	string rdata;
};

struct dnsinfo{
	unsigned char buf[BUFLEN];
	time_t ttl;
	time_t time_entered;
};

map<string, dnsinfo> cache;

int get_header(dnsheader* h, unsigned char*buf, int* pos);
int get_query(dnsquery* q, unsigned char* buf, int* pos);
int get_response(dnsresponse *r, unsigned char* buf, int* pos);
bool check_cache(string name);
void unset_recursion_bit(unsigned char* buf);
int valid_port(string s);
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

	unsigned char buf[BUFLEN];
	unsigned char recbuf[BUFLEN];
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
			sendto(sockfd, &cache[q.qname].buf, sizeof(cache[q.qname].buf), 0, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_in));
		}else{
			unset_recursion_bit(buf);
			sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&rootaddr,sizeof(struct sockaddr_in));
			alarm(2);
			if (recvfrom(sockfd, recbuf, BUFLEN, 0, (struct sockaddr*)&rootaddr, &rootLength) < 0){
				perror("Receive error");
				return 0;
			}
			alarm(0);

			bool found = false;
			stack<dnsresponse> ns_stack;
			vector<dnsresponse> ar_vector;
			while(!found){
				unset_recursion_bit(recbuf);
				dnsheader rh;
				dnsquery rq;
				pos = 0;

				if (get_header(&rh, recbuf, &pos) < 0){
					cerr << "Unable to get header info" << endl;
				}

				if (get_query(&rq, recbuf, &pos) < 0){
					cerr << "Unable to get query info" << endl;
				}

				/*for(int i = 0; i < sizeof(recbuf); i++){
 					printf("%02X ",recbuf[i]);
 				}*/

				int response_num = ntohs(rh.ancount) + ntohs(rh.nscount) + ntohs(rh.arcount);
				dnsresponse r[response_num];

				for(int i = 0; i < response_num; i++){
					cout << "Response " << i + 1 << endl;
					if (get_response(&(r[i]), recbuf, &pos) < 0){
						cerr << "Unable to get response info" << endl << endl;
					}
				}

				if(response_num > 0){
					for(int i = ntohs(rh.ancount); i < ntohs(rh.nscount); i++){//push name servers to stack
						ns_stack.push(r[i]);
					}
					for(int i = ntohs(rh.ancount) + ntohs(rh.nscount); i < response_num; i++){//push additional records to stack
						ar_vector.push_back(r[i]);					
					}				
				}

				if(ntohs(rh.ancount) > 0){
					cerr << "FOUND ANSWER!" << endl;
					found = true;
					for(int i = 0; i < ntohs(rh.ancount); i++){
						dnsinfo di;
						memcpy(&di.buf,&recbuf,BUFLEN);
						di.ttl = r[i].rttl;
						di.time_entered = time(NULL);
						cache[rq.qname] = di;
					}
					sendto(sockfd, recbuf, BUFLEN, 0, (struct sockaddr*)&clientaddr,sizeof(struct sockaddr_in));//send answers back to client
				}else{
					bool match = false;
					string nextaddr;
					while(!match && !found){
						dnsresponse ns = ns_stack.top();
						ns_stack.pop();
						for(int i = 0; i < ar_vector.size(); i++){
							if(strcmp(ns.rdata.c_str(),ar_vector[i].rname.c_str()) == 0){
								nextaddr = ar_vector[i].rdata;
								match = true;
							}
						}
						if(ns_stack.empty() && !match){
							cerr << "Ran out of name servers to check" << endl;
							return -1;
						}
					}

					struct sockaddr_in nsaddr;
					nsaddr.sin_family = AF_INET;
					nsaddr.sin_port = htons(53);
					nsaddr.sin_addr.s_addr = inet_addr(nextaddr.c_str());
					socklen_t nslength = sizeof(nsaddr);

					memset(recbuf, 0, sizeof(buf));
					sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&nsaddr,sizeof(struct sockaddr_in));//send answers back to client
					if (recvfrom(sockfd, recbuf, BUFLEN, 0, (struct sockaddr*)&nsaddr, &nslength) < 0){
						perror("Receive error");
						return 0;
					}
				}
			}
		}

		memset(buf, 0, sizeof(buf));

		ofstream cacheOut;
		cacheOut.open ("cache.txt");

		for(const auto& item : cache){
			cacheOut << item.first << "\n";
		}
		cacheOut.close();
	}

	return 0;
}

int get_header(dnsheader* h, unsigned char* buf, int* pos){
	int start = *pos;
	memcpy(h, buf, 12);
	*pos = 12;
	cout << "DNS Header" << endl;
	cout << "------------------------------------" << endl;
	printf("ID: 0x%02X\n", ntohs(h->id));
	printf("Flags: 0x%02X\n", ntohs(h->flags));
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

int get_query(dnsquery* q, unsigned char* buf, int* pos){
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

	for(int i = start; i < *pos; i++){
 		printf("%02X ",buf[i]);
 	}
 	cout << endl << endl;

	return 0;
}

int get_response(dnsresponse* r, unsigned char* buf, int* pos){
	int start = *pos;
	char tempchar;
	string name;
	uint16_t length;
	uint16_t ofs = 0; //offset
	int cmpcnt = 0; //compression count
	memcpy(&length, &buf[(*pos)++], 1);
	while(length != 0){
		if(length >= COMPRESSION){
			cmpcnt++;
			if(cmpcnt > 1){
				memcpy(&ofs, &buf[ofs-1], 2);
				ofs = ntohs(ofs);
				ofs &= 16383;
				memcpy(&length, &buf[ofs++], 1);
			}else{
				memcpy(&ofs, &buf[(*pos)-1], 2);
				(*pos)++;
				ofs = ntohs(ofs);
				ofs &= 16383;
				memcpy(&length, &buf[ofs++], 1);
			}
		}else if(ofs != 0){
			for(int i = 0; i < length; i++){
				memcpy(&tempchar, &buf[ofs++], 1);
	          	name += tempchar;
			}
			name += ".";
	        memcpy(&length, &buf[ofs++], 1);
		}else{
			for(int i = 0; i < length; i++){
				memcpy(&tempchar, &buf[(*pos)++], 1);
	          		name += tempchar;
			}
			name += ".";
	        memcpy(&length, &buf[(*pos)++], 1);
		}
	}
	r->rname += name;
	//memcpy(&(r->rname),&name,sizeof(name));
	memcpy(&(r->rtype),&buf[*pos],2);
	*pos += 2;
	memcpy(&(r->rclass),&buf[*pos],2);
	*pos += 2;

	// char tempbuf[4];
	// memcpy(&tempbuf[0],&buf[*pos],2); //memcpy(&(r->rttl),&buf[*pos],2);
	// memcpy(&tempbuf[2],&buf[*pos+2],2);
	// memcpy(&(r->rttl),&tempbuf[0],4);
	// memcpy(&(r->rttl),&buf[*pos],2);
	//
	// *pos += 4;
	r->rttl =((buf[*pos+0] << 24)
				+ (buf[*pos+1] << 16)
				+ (buf[*pos+2] << 8)
				+ (buf[*pos+3]));
	*pos+=4;
	memcpy(&(r->rdlength),&buf[*pos],2);
	*pos += 2;

	if(ntohs(r->rtype) == 1){
		uint8_t tempint;
		for(int i = 0; i < 4; i++){
			memcpy(&tempint, &buf[(*pos)++], 1);
	          	r->rdata += to_string(tempint);
			if(i < 3){
				r->rdata += ".";
			}
		}
	}else if(ntohs(r->rtype) == 2 || ntohs(r->rtype) == 5){
		name = "";
		ofs = 0; //offset
		cmpcnt = 0; //compression count
		memcpy(&length, &buf[(*pos)++], 1);
		while(length != 0){
			if(length >= COMPRESSION){
				cmpcnt++;
				if(cmpcnt > 1){
					memcpy(&ofs, &buf[ofs-1], 2);
					ofs = ntohs(ofs);
					ofs &= 16383;
					memcpy(&length, &buf[ofs++], 1);
				}else{
					memcpy(&ofs, &buf[(*pos)-1], 2);
					(*pos)++;
					ofs = ntohs(ofs);
					ofs &= 16383;
					memcpy(&length, &buf[ofs++], 1);
				}
			}else if(ofs != 0){
				for(int i = 0; i < length; i++){
					memcpy(&tempchar, &buf[ofs++], 1);
	          			name += tempchar;
				}
				name += ".";
	        	memcpy(&length, &buf[ofs++], 1);
			}else{
				for(int i = 0; i < length; i++){
					memcpy(&tempchar, &buf[(*pos)++], 1);
	          			name += tempchar;
				}
				name += ".";
	        		memcpy(&length, &buf[(*pos)++], 1);
			}
		}
		//memcpy(&(r->rdata),&name,sizeof(name));
		r->rdata += name;
	}else{
		cerr << "Incompatible type" << endl;
		return -1;
	}

	cout << "RNAME: " << r->rname << endl;
	cout << "RTYPE: " << ntohs(r->rtype) << endl;
	cout << "RCLASS: " << ntohs(r->rclass) << endl;
	cout << "RTTL: " << r->rttl << endl;
	cout << "RDLENGTH: " << ntohs(r->rdlength) << endl;
	cout << "RDATA: " << r->rdata << endl;

	for(int i = start; i < *pos; i++){
 		printf("%02X ",buf[i]);
 	}

 	cout << endl << endl;

	return 0;
}

// Check if name is in cache
bool check_cache(string name){
	dnsinfo info = cache[name];
	if(cache.count(name) != 0){
		if(time(NULL) > (info.ttl + info.time_entered)){
			cache.erase(name);
			return false;
		}

		/*cout << "The current cache is:\n";
		for(map<string,dnsinfo>::iterator it = cache.begin(); it != cache.end(); it++){
			cout << it->second.dr.rname << endl;
		}*/
		return true;
	}
	else
		return false;
}

void unset_recursion_bit(unsigned char* buf){
	uint16_t temp = 254; //1111111011111111 the 0 is the RD bit
	buf[2] &= temp;
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
