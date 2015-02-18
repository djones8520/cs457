//note that this code relies on c++11 features, and thus must be
//compiled with the -std=c++11 flag when using g++

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <signal.h>
#include <unistd.h>

#define TYPE_A 1
#define CLASS_IN 1

using namespace std;

struct dnsheader{
  uint16_t id;
  uint16_t flags;
  uint16_t qcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
};

struct dnsresponse{
  uint16_t type;
  uint16_t dns_class;
  uint32_t ttl;
  uint16_t rdlength;
  uint8_t* rdata;
  string name;
};

//string getName(uint8_t line[512], int* pos);
void CatchAlarm(int);

/*
 * Still left to do
 * ----------------------
 * Print out response header (I forgot how to print out in hex format for id and flags)
 * Print out response (type and class have set values and TTL should be output in days)
 * Need to timeout after 2 seconds if there is no response from the DNS server
 *
 * Link to RFC 1035: https://www.ietf.org/rfc/rfc1035.txt
 */

int main(int argc, char** argv){
	string ipaddress;

	if(argc > 1)
		ipaddress = argv[1];
	else{
		ifstream resolv ("/etc/resolv.conf");

		if(resolv.is_open()){
			string line;

			while(getline(resolv, line)){
				cout << line << endl;
				if(line.find("nameserver") != string::npos)
					ipaddress = line.substr(11);
			}

			resolv.close();
		}
		else{
			cout << "Error reading file." << endl;
			return 0;
		}
	}

  int sockfd = socket(AF_INET,SOCK_DGRAM,0);
  if(sockfd<0){
    cout<<"There was an error creating the socket"<<endl;
    return 1;
  }

  struct sockaddr_in serveraddr;
  serveraddr.sin_family=AF_INET;
  serveraddr.sin_port=htons(53);
  serveraddr.sin_addr.s_addr=inet_addr(&ipaddress[0]);

  struct timeval to;
  to.tv_sec=5;
  to.tv_usec=0;
  setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to));

  cout <<"Enter a domain name: ";

  string domain;
  getline(cin,domain);
  if(domain.back()!='.'){
    domain+='.';
  }

  unsigned seed = chrono::system_clock::now().time_since_epoch().count();
  default_random_engine generator(seed);
  uniform_int_distribution<u_int16_t> distribution(0,65535);

  char buf[512];
  dnsheader qh;
  qh.id = distribution(generator);
  uint16_t flags = 0;
  flags |= (1<<8);
  qh.flags = htons(flags);
  qh.qcount = htons(1);
  qh.ancount = htons(0);
  qh.nscount = htons(0);
  qh.arcount = htons(0);

  cout << "Request Header" << endl;
  cout << "------------------------------------" << endl;
  cout << "ID: 0x" << hex << ntohs(qh.id) << endl;
  cout << "Flags: 0x" << hex << ntohs(qh.flags) << endl;
  cout << "QCOUNT: " << ntohs(qh.qcount) << endl;
  cout << "ANCOUNT: " << ntohs(qh.ancount) << endl;
  cout << "NSCOUNT: " << ntohs(qh.nscount) << endl;
  cout << "ARCOUNT: " << ntohs(qh.arcount) << endl << endl;

  memcpy(buf,&qh,12);
  int pos=12;
  istringstream domainstream(domain);
  string label;
  while(getline(domainstream,label,'.').good()){
    buf[pos++]=label.length();
    strncpy(&buf[pos],label.c_str(),label.length());
    pos+=label.length();
  }
  buf[pos++]=0;
  buf[pos++]=0;
  buf[pos++]=TYPE_A;
  buf[pos++]=0;
  buf[pos++]=CLASS_IN;
  int tmp = pos;
  pos = 0;
  for(int i = 0; i < tmp; i++){
    printf("%02X ",ntohs(buf[i]));
  }

  cout << endl << endl;
  cout << "Sent our query" << endl << endl;

  sendto(sockfd,buf,tmp,0,
	 (struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in));

  uint8_t line[512];
  signal(SIGALRM, CatchAlarm);
  alarm(2);
  recvfrom(sockfd,line,512,0,(struct sockaddr*)&serveraddr,(unsigned int*)sizeof(serveraddr));
  alarm(0);
  dnsheader rh;
  memcpy(&rh,line,12);

  cout << "Response Header" << endl;
  cout << "------------------------------------" << endl;
  cout << "ID: 0x" << hex << ntohs(rh.id) << endl;
  cout << "Flags: 0x" << hex << ntohs(rh.flags) << endl;
  cout << "QCOUNT: " << ntohs(rh.qcount) << endl;
  cout << "ANCOUNT: " << ntohs(rh.ancount) << endl;
  cout << "NSCOUNT: " << ntohs(rh.nscount) << endl;
  cout << "ARCOUNT: " << ntohs(rh.arcount) << endl << endl;
  pos = 0;
  for(unsigned int i = 0; i < sizeof(line); i++){
    printf("%02X ",line[i]);
  }
  cout << endl << endl;
  int num_responses = ntohs(rh.ancount) + ntohs(rh.nscount) + ntohs(rh.arcount);
  
  // count of entries in answer
  int count = 0;
  dnsresponse answer[num_responses];
  pos = 12;

  for(int i = 0; i < ntohs(rh.qcount); i++){
    dnsresponse question;
    string name;
    short length;
    char tempchar;
    if((ntohs(line[pos]) & 11000000) == 11000000){ //if the length octet starts with 1 1, then the following value is an offset pointer
      pos = ntohs(line[pos]);
      memcpy(&length,&line[pos],1);
      pos++;
      while(length != 0){
        for(int i = 0; i < length; i++){
          tempchar = (char)(ntohs(line[pos++]));
          name += tempchar;
        }
        name += ".";
        memcpy(&length,&line[pos],1);
        pos++;
      }
    }else{
      memcpy(&length,&line[pos],1);
      pos++;
      while(length != 0){
        for(int i = 0; i < length; i++){
          tempchar = (char)(line[pos++]);
          name += tempchar;
        }
        name += ".";
        memcpy(&length,&line[pos],1);
        pos++;
      }
    }
    memcpy(&question,&line[pos],4);
    question.name = name;
    answer[i] = question;
  }

  for (int i = 0; i < (rh.qcount); i++){
    cout << "Question:" << endl;
    cout << "Name: "  << answer[i].name << endl;
    cout << "Type: "  << ntohs(answer[i].type) << endl;
    cout << "Class: "  << ntohs(answer[i].dns_class) << endl;
  }

  //loops through the responses creating a dnsresponse struct for each and puts them all into an array
  for(int i = ntohs(rh.qcount); i <= num_responses; i++){
    dnsresponse r;
    string name;
    short length;
    char tempchar;
    if((ntohs(line[pos]) & 11000000) == 11000000){ //if the length octet starts with 1 1, then the following value is an offset pointer
      pos = ntohs(line[pos]);
      memcpy(&length,&line[pos],1);
      pos++;
      while(length != 0){
        for(int i = 0; i < length; i++){
          tempchar = (char)(ntohs(line[pos++]));
          name += tempchar;
        }
        name += ".";
        memcpy(&length,&line[pos],1);
        pos++;
      }
    }else{
      memcpy(&length,&line[pos],1);
      pos++;
      while(length != 0){
        for(int i = 0; i < length; i++){
          tempchar = (char)(line[pos++]);
          name += tempchar;
        }
        name += ".";
        memcpy(&length,&line[pos],1);
        pos++;
      }
    }
    memcpy(&r,&line[pos],10);
    pos += 10;

    if(ntohs(r.type) == TYPE_A){
	uint8_t data[r.rdlength];
    	for(int j = 0; j < r.rdlength; j++){
      		data[j] = line[pos++];
   	}

	r.rdata = data;
	r.name = name;
	answer[i] = r;
	count++;
    }
    
  }

  for (int i = ntohs(rh.qcount); i <= num_responses; i++){
    cout << "Answer:" << endl;
    cout << "Name: "  << answer[i].name << endl;
    cout << "Type: "  << ntohs(answer[i].type) << endl;
    cout << "Class: "  << ntohs(answer[i].dns_class) << endl;
    //cout << "TTL:"  << r.ttl << endl;
    cout << "Data Length: "  << ntohs(answer[i].rdlength) << endl;
    //cout << "Data:"  << r.name << endl;
  }

  return 0;
}


void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    cout << "Server took too long to respond\nQuitting...\n";
    exit(1);
}
