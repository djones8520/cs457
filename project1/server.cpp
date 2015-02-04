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
#include <sys/stat.h>
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

#define BYTES_TO_SEND 256

typedef	struct requestParams{
	int clientsocket;
	string data;
} request;

void* httpRequest(void* arg);
void* sendErrorStatus(int statusCode,int* clientsocket);
string makeDateHeader();
string makeLastModifiedHeader(string);
string makeContentTypeHeader(string filename);
string makeContentLengthHeader(int length);
int isValidFileName(string);

int main(int argc, char **argv){
	cout << "Last edited: " << makeLastModifiedHeader("test.txt") << endl;
	cout << "Date: " << makeDateHeader() << endl;
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

	while (1){
		pthread_t thread;
		void *result;
		int status;

		socklen_t len = sizeof(clientaddr);
		//g++ doesn't like this
		int clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);
		   
		char line[5000];
		recv(clientsocket, line, 5000, 0);
		cout << "Requested file from client: " << line << endl;

		requestParams *req = new requestParams;
		
		string requestData = line;
		req->clientSocket = clientsocket;
		req->data = requestData;

		//char str[INET_ADDRSTRLEN];
		//inet_ntop(AF_INET, &clientaddr, &str, INET_ADDRSTRLEN);
		//printf("A client connected (IP=%s : Port=9010)\n", str);

		if((status = pthread_create(&thread, NULL, httpRequest, &req)) != 0){
			cout << "Error creating thread" << endl;
		}
	}
	return 0;
}


/***********************************************************
 *
 ***********************************************************/
void* httpRequest(void* arg){
	//char line[5000];
	//int sockfd = *(int *) arg;
        //int n;

	cout << "Thread created" << endl;
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
    string filepath = docroot;
    filepath+="/";
    filepath+=filename;
    string responseHeader;
    FILE *fp = fopen(filepath,"rb");
	if(fp == NULL){
		cout << "IOError: could not open " << filepath << "\n";
		sendErrorStatus(404,&clientsocket);
		exit(1);
	}

	responseHeader = "HTTP/1.1 200 OK\r\n";
	responseHeader+=makeDateHeader();
	responseHeader+=makeLastModifiedHeader(filename);
	responseHeader+=makeContentTypeHeader(filename);
	
	cout << "Sending " << filename << "\n";

	while(1){
		char buff[BYTES_TO_SEND]={0};
		int bytesRead = fread(buff,1,BYTES_TO_SEND,fp);
		string response = responseHeader;
		if(bytesRead > 0){
			response+=makeContentLengthHeader(bytesRead);
			response+="\r\n";
			response+=buff;
			send(clientsocket,response,sizeof(response),0);
		}

		if(bytesRead < BYTES_TO_SEND){
			if(feof(fp))
				cout << "Reached end of file\n";
			else if(ferror(fp))
				cout << "Error while reading file\n";
			break;
		}
	}

	//close(sockfd);
	pthread_detach(pthread_self());
}

/***********************************************************
 * Sends response header with appropriate status code.
 ***********************************************************/
void* sendErrorStatus(int statusCode,int* clientsocket){
	string response;
	switch(statusCode){
		case 304:
			response = "HTTP/1.1 304 Page hasn't been modified\r\n\r\n";
			send(*clientsocket,response,sizeof(response),0);
			break;
		case 404:
			response = "HTTP/1.1 404 Page not found\r\n\r\n";
			send(*clientsocket,response,sizeof(response),0);
			break;
		case 501:
			response = "HTTP/1.1 501 POST requests not implemented\r\n\r\n";
			send(*clientsocket,response,sizeof(response),0);
			break;	
	}	
}

/***********************************************************
 * Returns the current time in RFC1123 formatting. All 
 * status codes should be checked before calling this
 * function. Specifically, if a 500 level code was sent, do
 * not send the date header. 
 ***********************************************************/
string makeDateHeader()
{
	const char *DAY_NAMES[] =
  	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	const char *MONTH_NAMES[] =
  	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    	
	const int RFC1123_TIME_LEN = 29;
    	time_t t;
    	struct tm tm;
    	char * buf = (char *)malloc(RFC1123_TIME_LEN+1);
	string complete_date;

    	time(&t);
    	gmtime_r(&t, &tm);

    	strftime(buf, RFC1123_TIME_LEN+1, "---, %d --- %Y %H:%M:%S GMT", &tm);
    	memcpy(buf, DAY_NAMES[tm.tm_wday], 3);
    	memcpy(buf+8, MONTH_NAMES[tm.tm_mon], 3);

	complete_date = "Date: ";
        complete_date += buf;

        return complete_date;
}


/***********************************************************
 * Given a file name of a file that exist, returns the date 
 * that the file was last modified as an RFC1123 format. The
 * last-modified header should be sent with all file
 * requests, regardless of status code.
 ***********************************************************/
string makeLastModifiedHeader(string file_name)
{
	const char *DAY_NAMES[] =
        { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        const char *MONTH_NAMES[] =
        { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	string complete_date;
	struct stat attr;
	struct tm tm;
	const int RFC1123_TIME_LEN = 29;
	char * buf = (char *)malloc(RFC1123_TIME_LEN+1);

	stat(file_name.c_str(),&attr);
        gmtime_r(&attr.st_mtime,&tm);

	strftime(buf,RFC1123_TIME_LEN+1, "---, %d --- %Y %H:%M:%S GMT", &tm);
	memcpy(buf, DAY_NAMES[tm.tm_wday], 3);
        memcpy(buf+8, MONTH_NAMES[tm.tm_mon], 3);
	complete_date = "Last-Modified: "; 
	complete_date += buf;
	
	return complete_date;
}


/***********************************************************
 * Creates the Content-Type header for the response header. Checks if
 * the requested filename's extension is .html, .jpeg, or .pdf and then
 * creates header accordingly. Otherwise defaults to text.
 ***********************************************************/
string makeContentTypeHeader(string filename){
	char *str1 = (char*)filename.c_str();
	strtok(str1,".");
	char *str2 = strtok(NULL,".");
	string header = "Content-Type:";
	if(strcmp(str2,"html")==0){
		header+="text/html\r\n";
	}else if(strcmp(str2,"jpeg")==0){
		header+="image/jpeg\r\n";
	}else if(strcmp(str2,"pdf")==0){
		header+="application/pdf\r\n";
	}else{
		header+="text/plain\r\n";
	}
	return header;
}


/**************************************************************
 * Creates the Content-Length header for the response header.
 **************************************************************/
string makeContentLengthHeader(int length){
	string header = "Content-Length:";
	header+=length;
	header+="\r\n"
	return header;
}


/**************************************************************
 * Check if the filename is valid. A positive integer 
 * indicates the file is valid and access is allowed, 0 
 * indicates the file does not exist, and a negative number 
 * means that the filename is not valid.
 *
 * Valid file names are az AZ . -
 * .. is invalid
 **************************************************************/
int isValidFileName(string file_name)
{
	

	return 0;
}
