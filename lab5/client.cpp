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
  u_int16_t id;
  u_int16_t flags;
  u_int16_t qcount;
  u_int16_t ancount;
  u_int16_t nscount;
  u_int16_t arcount;
};

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
  dnsheader d;
  d.id = distribution(generator);
  u_int16_t flags = 0;
  flags |= (1<<8);
  d.flags = htons(flags);
  d.qcount = htons(1);
  d.ancount = htons(0);
  d.nscount = htons(0);
  d.arcount = htons(0);

  memcpy(buf,&d,12);
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
  return 0;
}