/****************************************************************
* Project 1: Mini HTTP Server
* Due 2/4/2015
*
* Server File
*
* Michael Kinkema
* Chase	Pietrangelo
* Danny	Selgo
* Daniel Jones
*
****************************************************************/

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;


typedef	struct requestParams{
			int clientSocket;
			string line;
		} request;

void* httpRequest(void* arg);
string makeDateHeader();
string makeContentTypeHeader(string filename);

int main(int argc, char **argv){
	int port;
	char *docroot = (char*)malloc(1024);
	char *logfile;

	port = 8080;
	getcwd(docroot,1024);
	if (argc > 1){
		for (int i = 1; i < argc; i++){
			if (strcmp(argv[i], "-p") == 0){
				i++;
				port = atoi(argv[i]);
			}
			else if (strcmp(argv[i], "-docroot") == 0){
				i++;
				docroot = argv[i];
			}
			else if (strcmp(argv[i], "-logfile") == 0){
				i++;
				logfile = argv[i];
				freopen(argv[i], "w", stdout);
			}
			else{
				cerr << "Invalid option. Valid options are -p, -docroot, -logfile\n";
				return 1;
			}
		}
	}
	printf("port: %d\n",port);
	printf("docroot: %s\n",docroot);
	printf("logfile: %s\n",logfile);
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0){
		printf("Problem creating socket\n");
		return 1;
	}

	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(8080);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
	listen(sockfd, 10);

	char line[5000];
	while (1){
		pthread_t thread;
		void *result;
		int status;

		socklen_t len = sizeof(clientaddr);
		//g++ doesn't like this
		int clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);
		   
		char line[5000];
		recv(clientsocket, line, 5000, 0);
		printf("Requested file from client: %s\n",line);
		

		requestParams *req = new requestParams;
		
		string line2;
		req->clientSocket = clientsocket;
		req->line = line2;

		//char str[INET_ADDRSTRLEN];
		//inet_ntop(AF_INET, &clientaddr, &str, INET_ADDRSTRLEN);
		//printf("A client connected (IP=%s : Port=9010)\n", str);


		if((status = pthread_create(&thread, NULL, httpRequest, &req)) != 0){
			fprintf(stderr, "thread create error %d: %s\n", status, strerror(status));
		}
	}

	return 0;
}

void* httpRequest(void* arg){
	//char line[5000];
	//int sockfd = *(int *) arg;
        //int n;

	cout << "Thread created";
/*
        while((n = recv(sockfd,line,5000,0))>0){
            printf("%s\n",line);
            memset(line,0,sizeof(line));
        }
        if(n<0){
            printf("Sorry, had a problem receiving.\n");
            exit(1);
        } else {
            printf("Server has disconnected.\n");
            exit(1);
        }
*/
	//close(sockfd);
	pthread_detach(pthread_self());
}


string makeDateHeader()
{
        /*Doesn't send date header if there is a server error
        if(response_status >= 500 && response_status <= 599)
        {
                return "0"
        }*/

	const char *DAY_NAMES[] =
  	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	const char *MONTH_NAMES[] =
  	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    	
	const int RFC1123_TIME_LEN = 29;
    	time_t t;
    	struct tm tm;
    	char * buf = (char *)malloc(RFC1123_TIME_LEN+1);

    	time(&t);
    	gmtime_r(&t, &tm);

    	strftime(buf, RFC1123_TIME_LEN+1, "---, %d --- %Y %H:%M:%S GMT", &tm);
    	memcpy(buf, DAY_NAMES[tm.tm_wday], 3);
    	memcpy(buf+8, MONTH_NAMES[tm.tm_mon], 3);

    return buf;
}

/*
 * Creates the Content-Type header for the response header. Checks if
 * the requested filename's extension is .html, .jpeg, or .pdf and then
 * creates header accordingly. Otherwise defaults to text.
 */
string makeContentTypeHeader(string filename){
	char *str1 = (char*)filename.c_str();
	strtok(str1,".");
	char *str2 = strtok(NULL,".");
	string header = "Content-Type:";
	if(strcmp(str2,"html")==0){
		header+="text/html;";
	}else if(strcmp(str2,"jpeg")==0){
		header+="image;";
	}else if(strcmp(str2,"pdf")==0){
		header+="application/pdf;";
	}else{
		header+="text;";
	}
	return header;
}

