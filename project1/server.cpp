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
#include <vector>

using namespace std;

#define BYTES_TO_SEND 256

/*
 clientsocket stores the connection
 data holds the request from the client
 */
struct requestParams{
    int clientsocket;
    string data;
};

void* httpRequest(void* arg);
void sendErrorStatus(int statusCode,int* clientsocket,string logmsg);
string makeDateHeader();
string makeLastModifiedHeader(string);
string makeContentTypeHeader(string filename);
string makeContentLengthHeader(long length);
int checkIfModifiedSince(string);
vector<string> explode(const string& str, const char& ch);
long getFileSize(string filename);
int isValidFileName(string);

int main(int argc, char **argv){
    int port;
    char *logfile;
    port = 8080;
    
    if (argc > 1){
        for (int i = 1; i < argc; i++){
            if (strcmp(argv[i], "-p") == 0){
                i++;
                port = atoi(argv[i]);
            }
            else if (strcmp(argv[i], "-docroot") == 0){
                i++;
                if(chdir(argv[i])<0){
                    cerr << "Could not change working directory to given directory.\n";
                    return 1;
                }
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
    
    char *docroot = (char*)malloc(1024);
    getcwd(docroot,1024);
    
    printf("port: %d\n", port);
    printf("docroot: %s\n\n", docroot);
    //printf("logfile: %s\n", logfile);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0){
        printf("Problem creating socket\n");
        return 1;
    }
    
    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    
    struct timeval to;
    to.tv_sec=20;
    to.tv_usec=0;

    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&to,sizeof(to));

    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0)
        cout << "setsockopt failed" << endl;

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &to,  sizeof(to)) < 0)
        cout << "setsockopt failed" << endl;

    bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
    listen(sockfd, 10);
    
    while (1){
        pthread_t thread;
        void *result;
        int status;
        
        socklen_t len = sizeof(clientaddr);
        int clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);
        
        char line[5000];
        recv(clientsocket, line, 5000, 0);
        //cout << "Request:" << endl << line << endl;
        
        requestParams *req = new requestParams;
        
        string requestData = line;
        req->clientsocket = clientsocket;
        req->data = requestData;
        
        //char str[INET_ADDRSTRLEN];
        //inet_ntop(AF_INET, &clientaddr, &str, INET_ADDRSTRLEN);
        //printf("A client connected (IP=%s : Port=9010)\n", str);
        
        if((status = pthread_create(&thread, NULL, httpRequest, req)) != 0){
            cerr << "Error creating thread" << endl;
        }
    }
    return 0;
}


/***********************************************************
 * Thread function that takes HTTP requests to the server,
 * parses the request, and uses the parsed data to create an
 * appropriate response header before sending the requested
 * file.
 ***********************************************************/
void* httpRequest(void* arg){
    requestParams* req = (requestParams*) arg;

    string logmsg = "Request:\n";
    logmsg+=req->data;
    
    vector<string> parsed = explode(req->data, ' ');
    if(strcmp(parsed[0].c_str(),"GET")!=0){
        sendErrorStatus(501,&req->clientsocket,logmsg);
        return 0;
    }
    string filename = parsed[1];
    filename.erase(0,1);
    
    string filepath;
    filepath+=filename;
    
    if(isValidFileName(filepath) != 1)
      {
	logmsg+="IOError: could not open ";
        logmsg+=filepath;
	logmsg+="\n\n";
	cout << "Invalid File Requested" << filepath << endl << endl;
        sendErrorStatus(404, &req->clientsocket,logmsg);
        return 0;
      }

    string responseHeader;
    FILE *fp = fopen(&filepath[0], "rb");
    if(fp == NULL){
        logmsg+="IOError: could not open ";
        logmsg+=filepath;
	logmsg+="\n\n";
        sendErrorStatus(404, &req->clientsocket,logmsg);
        return 0;
    }

    long fileSize = getFileSize(filename);
    
    responseHeader = "HTTP/1.1 200 OK\r\n";
    responseHeader+=makeDateHeader();
    responseHeader+=makeLastModifiedHeader(filename);
    responseHeader+=makeContentTypeHeader(filename);
    responseHeader+=makeContentLengthHeader(fileSize);
    responseHeader+= "\r\n";
    logmsg+="Response:\n";
    logmsg+=responseHeader;
    cout << logmsg;
    write(req->clientsocket, responseHeader.c_str(), responseHeader.size());
    while(1){
        char buff[BYTES_TO_SEND]={0};
        int bytesRead = fread(buff,1,BYTES_TO_SEND,fp);
        if(bytesRead > 0){
            write(req->clientsocket, buff, sizeof(buff));  //Sends the file without the header
        }
        
        if(bytesRead < BYTES_TO_SEND){
            if(ferror(fp))
	      cerr << "Error while reading file: " << filename << endl;;
            break;
        }
    }
    
    free(fp);
    pthread_detach(pthread_self());

    return 0;
}

/***********************************************************
 * Sends response header with appropriate status code.
 * If 404 error, will send 404.html to client.
 ***********************************************************/
void sendErrorStatus(int statusCode,int* clientsocket,string logmsg){
    string responseHeader;
    switch(statusCode){
        case 304:
            responseHeader = "HTTP/1.1 304 Page hasn't been modified\r\n\r\n";
	    logmsg+="304Response:\n"; 
	    logmsg+=responseHeader;
	    cout << logmsg;
            write(*clientsocket, responseHeader.c_str(), responseHeader.size());
            break;
        case 404:
        {
            string filename;
            filename = "404.html";
            FILE *fp;
            fp = fopen(filename.c_str(),"rb");
            if(fp == NULL){
                logmsg+="IOError: could not open ";
		logmsg+=filename;
		logmsg+="\n";
                break;
            }
            
            responseHeader = "HTTP/1.1 404 Page_not_found\r\n";
            responseHeader+=makeDateHeader();
	    responseHeader+=makeLastModifiedHeader(filename);
	    responseHeader+=makeContentTypeHeader(filename);
	    responseHeader+=makeContentLengthHeader(sizeof(*fp));
	    responseHeader+= "\r\n";
	    logmsg+="404Response:\n";
	    logmsg+=responseHeader;
	    cout << logmsg;
	    write(*clientsocket, responseHeader.c_str(), responseHeader.size());
            
            while(1){
	      char buff[BYTES_TO_SEND]={0};
	      int bytesRead = fread(buff,1,BYTES_TO_SEND,fp);
	      if(bytesRead > 0){
		write(*clientsocket, buff, sizeof(buff));  //Sends the file without the header
	      }
        
	      if(bytesRead < BYTES_TO_SEND){
		if(ferror(fp))
		  cerr << "Error while reading file\n";
		break;
	      }
	    }
        }
            break;
        case 501:
            responseHeader = "HTTP/1.1 501 POST requests not implemented\r\n\r\n";
	    logmsg+="501Response:\n"; 
	    logmsg+=responseHeader;
	    cout << logmsg;
            write(*clientsocket, responseHeader.c_str(), responseHeader.size());
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
    complete_date += "\r\n";
    
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
    complete_date += "\r\n";
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
    string header = "Content-Type: ";
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
string makeContentLengthHeader(long length){
    string header = "Content-Length:";
    header += to_string(length);
    header +="\r\n";

    return header;
}


/**************************************************************
 * Checks if the filename is valid.
 *
 * A positive integer indicates the file is valid
 * 0 indicates the file does not exist
 * A negative integer indicates that the filename is not valid.
 * 
 * Valid file names are az AZ . - /
 * .. is invalid
 * ~ is invalid
 **************************************************************/
int isValidFileName(string file_name)
{
    struct stat buf;
    int i;
    
    //Check filename is valid
    for(i = 0; i < file_name.length(); i++)	
    {
        //a-z A-Z . - /
        if (!(file_name.at(i) >= 45 && file_name.at(i) <=57) && (file_name.at(i) >= 65 && file_name.at(i) <= 90)){
            return -1;
        }  
        //Check for ..
        else if(file_name.at(i) == 46){
            if(i != (file_name.length()-1)){
                if (file_name.at(i+1) == 46)
                    return -1;
            }
        }
    }
    
    
    //Check if file exist
    if(stat(file_name.c_str(), &buf) == 0)
      {
	if(S_ISREG(buf.st_mode))
	  return 1;
	return 0;
      }
    return 0;
}


/**************************************************************
 * Check if the file requested has been modified since the date
 * specified by the If-Modified-Since header.
 *
 * A negative integer indicates an invalid header.
 * 0 indicates the file has not been modified. A 304 error 
 *    should be sent in this case (not sent in this function).
 * A positive integer indicates that the file has not been
 *    modified.
 **************************************************************/
int checkIfModifiedSince(string time_header)
{/*
    	tm tm;
	string truncTime = truncTime.substr(19);

	//Sat, 29 Oct 1994 19:43:31 GMT
	strptime(truncTime,"%a, %d %b %Y %H:%M:%S");
   */ 
    	return 0;
}

/**************************************************************
 * Parses a string into a vector depending on what the given
 * delimiter is.
 **************************************************************/
vector<string> explode(const string& str, const char& ch) {
    string next;
    vector<string> result;
    
    for (string::const_iterator it = str.begin(); it != str.end(); it++) {
        if (*it == ch) {
            if (!next.empty()) {
                result.push_back(next);
                next.clear();
            }
        } else {
            next += *it;
        }
    }
    if (!next.empty())
        result.push_back(next);
    return result;
}

long getFileSize(string filename){
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);

    return rc == 0 ? stat_buf.st_size : -1;
}
