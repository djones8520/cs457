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
void* sendErrorStatus(int statusCode,int* clientsocket);
string makeDateHeader();
string makeLastModifiedHeader(string);
string makeContentTypeHeader(string filename);
string makeContentLengthHeader(int length);
int checkIfModifiedSince(string);
vector<string> explode(const string& str, const char& ch);

int main(int argc, char **argv){
    cout << "Last edited: " << makeLastModifiedHeader("test.txt") << endl;
    cout << "Date: " << makeDateHeader() << endl;
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
    printf("docroot: %s\n", docroot);
    //printf("logfile: %s\n", logfile);
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
        req->clientsocket = clientsocket;
        req->data = requestData;
        
        //char str[INET_ADDRSTRLEN];
        //inet_ntop(AF_INET, &clientaddr, &str, INET_ADDRSTRLEN);
        //printf("A client connected (IP=%s : Port=9010)\n", str);
        
        if((status = pthread_create(&thread, NULL, httpRequest, req)) != 0){
            cout << "Error creating thread" << endl;
        }
    }
    return 0;
}


/***********************************************************
 *
 ***********************************************************/
void* httpRequest(void* arg){
    requestParams* req = (requestParams*) arg;
    
    string filename = "PLACEHOLDER";
    
    cout << "Thread created" << endl;
    
    vector<string> parsed = explode(req->data, ' ');
    
    filename = parsed[1];
    filename.erase(0,1);
    cout << "FILE NAME: " << filename << endl;
    
    string filepath;// = docroot;
    //filepath+="/";
    filepath+=filename;
    string responseHeader;
    FILE *fp = fopen(&filepath[0], "rb");
    if(fp == NULL){
        cout << "IOError: could not open " << filepath << "\n";
        sendErrorStatus(404, &req->clientsocket);
        exit(1);
    }
    
    responseHeader = "HTTP/1.1 200 OK\r\n";
    responseHeader+=makeDateHeader();
    responseHeader+=makeLastModifiedHeader(filename);
    responseHeader+=makeContentTypeHeader(filename);
    responseHeader+=makeContentLengthHeader(sizeof(*fp));
    responseHeader+= "\r\n";
    cout << "Sending " << filename << "\n";
    send(req->clientsocket, responseHeader.c_str(), sizeof(responseHeader), 0);
    while(1){
        char buff[BYTES_TO_SEND]={0};
        int bytesRead = fread(buff,1,BYTES_TO_SEND,fp);
        if(bytesRead > 0){
            send(req->clientsocket, buff, sizeof(buff),0);  //Sends the file without the header
        }
        
        if(bytesRead < BYTES_TO_SEND){
            if(feof(fp))
                cout << "Reached end of file\n";
            else if(ferror(fp))
                cout << "Error while reading file\n";
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
void* sendErrorStatus(int statusCode,int* clientsocket){
    string response;
    switch(statusCode){
        case 304:
            response = "HTTP/1.1 304 Page hasn't been modified\r\n\r\n";
            send(*clientsocket, &response[0], sizeof(response),0);
            break;
        case 404:
        {
            string filename;
            filename = "404.html";
            FILE *fp;
            fp = fopen(filename.c_str(),"rb");
            if(fp == NULL){
                cout << "IOError: could not open " << filename << "\n";
                break;
            }
            string responseHeader;
            responseHeader = "HTTP/1.1 404 Page_not_found\r\n";
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
                    send(*clientsocket,response.c_str(),sizeof(response),0);
                }
                
                if(bytesRead < BYTES_TO_SEND){
                    if(feof(fp))
                        cout << "Reached end of file\n";
                    else if(ferror(fp))
                        cout << "Error while reading file\n";
                    break;
                }
            }
        }
            break;
        case 501:
            response = "HTTP/1.1 501 POST requests not implemented\r\n\r\n";
            send(*clientsocket, &response[0], sizeof(response), 0);
            break;
    }
	return 0;
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
    header+="\r\n";
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
    return 	stat(file_name.c_str(), &buf) == 0;
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
int checkIfModifiedSince(string pTime)
{
    
    
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