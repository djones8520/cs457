//note that this code relies on c++11 features, and thus must be
//compiled with the -std=c++11 flag when using g++

#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

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
  string name;
  uint16_t type;
  uint16_t dns_class;
  uint32_t ttl;
  uint16_t rdlength;
  uint8_t* rdata;
};

uint16_t convertFrom8To16(uint8_t dataFirst, uint8_t dataSecond);
uint32_t convertFrom16To32(uint16_t dataFirst, uint16_t dataSecond);
string getName(uint8_t line[512], int pos);

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

  int sockfd = socket(AF_INET,SOCK_DGRAM,0);
  if(sockfd<0){
    cout<<"There was an error creating the socket"<<endl;
    return 1;
  }

  struct sockaddr_in serveraddr;
  serveraddr.sin_family=AF_INET;
  serveraddr.sin_port=htons(53);
  serveraddr.sin_addr.s_addr=inet_addr("8.8.8.8");

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
  sendto(sockfd,buf,pos,0,
	 (struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in));
  cout<<"Sent our query"<<endl;

  uint8_t line[512];
  recvfrom(sockfd,line,512,0,(struct sockaddr*)&serveraddr,(unsigned int*)sizeof(serveraddr));

  //gets response header information
  dnsheader rh;
  pos = 0;
  rh.id = ntohs(convertFrom8To16(line[pos],line[pos++]));
  rh.flags = ntohs(convertFrom8To16(line[pos++],line[pos++]));
  rh.qcount = ntohs(convertFrom8To16(line[pos++],line[pos++]));
  rh.ancount = ntohs(convertFrom8To16(line[pos++],line[pos++]));
  rh.nscount = ntohs(convertFrom8To16(line[pos++],line[pos++]));
  rh.arcount = ntohs(convertFrom8To16(line[pos++],line[pos++]));

  int num_responses = rh.ancount + rh.nscount + rh.arcount;
  dnsresponse answer[num_responses];

  //loops through the responses creating a dnsresponse struct for each and puts them all into an array
  for(int i = 0; i < num_responses; i++){
    dnsresponse r;
    if(ntohs(line[pos+1]) & 11000000 == 1100000){ //if the length octet starts with 1 1, then the following value is an offset pointer
      r.name = getName(line,line[pos+=2]);
    }else{
      r.name = getName(line,pos++);
    }

    r.type = convertFrom8To16(line[pos++],line[pos++]);
    r.dns_class = convertFrom8To16(line[pos++],line[pos++]);
    r.ttl = convertFrom16To32(convertFrom8To16(line[pos++],line[pos++]),
                              convertFrom8To16(line[pos++],line[pos++]));
    r.rdlength = convertFrom8To16(line[pos++],line[pos++]);
    uint8_t data[r.rdlength];
    for(int j = 0; j < r.rdlength; j++){
      data[j] = line[pos++];
    }
    r.rdata = data;
    answer[i] = r;
  }


  return 0;
}

string getName(uint8_t line[512], int pos){
  string name;
  uint8_t length;
  while((length = ntohs(line[pos++])) != 0){
    for(uint8_t i = 0; i < length; i++){
      name += (char)ntohs(line[pos++]);
    }
    name += ".";
  }
  return name;
}

uint16_t convertFrom8To16(uint8_t dataFirst, uint8_t dataSecond) {
  uint16_t dataBoth = 0x0000;

  dataBoth = dataFirst;
  dataBoth = dataBoth << 8;
  dataBoth |= dataSecond;
  return dataBoth;
}

uint32_t convertFrom16To32(uint16_t dataFirst, uint16_t dataSecond) {
  uint32_t dataBoth = 0x00000000;

  dataBoth = dataFirst;
  dataBoth = dataBoth << 16;
  dataBoth |= dataSecond;
  return dataBoth;
}