// std libs
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
// my libs
#include "threads.h"
#include "protocols.h"
#include "manager.h"

#define ON_HOLD_COUNT 10

int serverRunning = 1;

void process(int sessionSock, void* usr) {
	User* user = (User*) usr;
	Header clientHeader;
	char buffer[4096];
	char redirect[128];
	int readHead = 0;
	int compare = 0;

	memset(&redirect, 0, 128);

	// read header from client
	int caught;
	while(1) {
		caught = recv(sessionSock, &buffer[readHead], 4096 - readHead, 0);
		if(caught < 0) { // error
			close(sessionSock);
			return;
		}
		if(caught > 0) {
			readHead += caught;
		}
		if(strstr(buffer, "\r\n\r\n")) { // we have reached the end of the HTTP header
			break;
		}
	}

	processHeader(buffer, &clientHeader);

	if(clientHeader.type == POST && clientHeader.contentLength > 0) {
		char* offset = strstr(buffer, "\r\n\r\n");
		char* data = offset + 4;
		if(strlen(offset) - 4 != clientHeader.contentLength) { // still need some post data
			while(1) {
				caught = recv(sessionSock, &buffer[readHead], clientHeader.contentLength - readHead, 0);
				if(caught < 0) { // error
					close(sessionSock);
					return;
				}
				if(caught > 0) {
					readHead += caught;
				}
				if(strstr(buffer, "\r\n\r\n")) { // we have reached the end of the HTTP header
					break;
				}
			}
		}
		buffer[clientHeader.contentLength] = '\0';
		if(fnmatch("username=*&password=*",  data, 0) == 0 && authenticate(user, data)) { // trying to login!
			sprintf(&redirect, "Location: /secure/\r\n", DOCUMENT_ROOT);
		}
		if(strstr(data, "signout=true")) { // trying to logging out!
			signout(user);
			sprintf(&redirect, "Location: /login.html\r\n", DOCUMENT_ROOT);
		}
	}

	// we can now use buffer as scrap space
	sprintf(buffer, "%s%s", DOCUMENT_ROOT, clientHeader.filename);
	memcpy(clientHeader.filename, buffer, strlen(buffer) + 1);
	char* status = STATUS_OK;
	char* contentType = CONTENT_TYPE_HTML;
	int isFile = 1; // 0 if dir list
	struct stat fileMeta;

	if(stat(clientHeader.filename, &fileMeta) == 0) {
		if(fileMeta.st_mode & S_IFDIR) { // is a dir
			if(buffer[strlen(buffer) - 1] != '/') {
				int end = strlen(buffer) - 1;
				buffer[end+1] = '/';
				buffer[end+2] = '\0';
			}
			sprintf(clientHeader.filename, "%s%s", buffer, "index.html");
			printf("%s\n", clientHeader.filename);
			if(stat(clientHeader.filename, &fileMeta) != 0) { // no index.html list dir
				clientHeader.filename[strlen(clientHeader.filename) - 10] = '\0'; // cut off index.html
				isFile = 0;
			}
			else {
				memcpy(clientHeader.fileType, "html", 5); // gotta make type html
			}
		}
	}
	else {
		status = STATUS_NOT_FOUND;
		isFile = 0;
		contentType = getContentType(&clientHeader);
	}
	if(isFile) {
		contentType = getContentType(&clientHeader);
	}

	// send response header to client
	if(redirect[0] != '\0') { // must send them somewhere else
		status = STATUS_REDIRECT;
	}
	if(strstr(clientHeader.filename, "secure/") && !isAuthenticated(user)) { // they can't do this!
		//status = STATUS_FORBIDDEN;
		sprintf(clientHeader.filename, "%s%s", DOCUMENT_ROOT, "/403.html");
		isFile = 1;
	}
	safeSend(sessionSock, status, strlen(status));
	safeSend(sessionSock, HEADER_SERVER, strlen(HEADER_SERVER));
	safeSend(sessionSock, contentType, strlen(contentType));
	if(redirect[0] != '\0') {
		safeSend(sessionSock, redirect, strlen(redirect));
	}
	if(contentType == CONTENT_TYPE_MP3) { // don't quit on big files!
		safeSend(sessionSock, HEADER_KEEP_ALIVE, strlen(HEADER_KEEP_ALIVE));
	}
	safeSend(sessionSock, HEADER_END, strlen(HEADER_END));

	printf("handeling %s\n", clientHeader.filename);

	// send content
	if(isFile) {
		int localFileFd = open(clientHeader.filename, 0, 0);
		int readVal = 0;
		int packet = 1;
		while((readVal = read(localFileFd, buffer, 4096)) != 0) { // read until eof
			//printf("sending packet %i of '%s' read: %i\n", packet, clientHeader.filename, readVal);
			packet++;
			safeSend(sessionSock, buffer, readVal);
		}

		close(localFileFd);
	}
	else { // file does not exist, so list dir or 404
		if(status == STATUS_NOT_FOUND) {
			char* tmp = malloc(1024);
			sprintf(tmp, "%s%s", DOCUMENT_ROOT, "/404.html");
			printf("%s\n", tmp);
			int localFileFd = open(tmp, 0, 0);
			if(localFileFd > 0) {
				int readVal = 0;
				int packet = 1;
				while((readVal = read(localFileFd, buffer, 4096)) != 0) { // read until eof
					//printf("sending packet %i of '%s' read: %i\n", packet, clientHeader.filename, readVal);
					packet++;
					safeSend(sessionSock, buffer, readVal);
				}
				close(localFileFd);
			}
			else {
				safeSend(sessionSock, "<b>404</b> Not Found", 20);
			}
			
		}
		else { // we have to list
			DIR* dir;
			struct dirent *item;
			int offset = 0;
			if((dir = opendir(clientHeader.filename)) != NULL) { // it shoudn't but just checking
				while((item = readdir(dir)) != NULL) {
					if(item->d_name[0] != '.') {
						sprintf(&buffer[offset], "<div><a href=\"./%s\">%s</a></div>\n", item->d_name, item->d_name);
						offset += (strlen(item->d_name) * 2) + 29; // 27 is the string length, now we override the \0 at the end
					}
				}
				closedir(dir);
				char* miniBuffer = malloc(strlen(buffer));
				memcpy(miniBuffer, buffer, strlen(buffer));
				char str[80];
				strcpy(str, DOCUMENT_ROOT);
				strcat(str, "/file-list.html");
				int listfd = open(str, 0, 0);
				if(listfd > 0) { // this file exists
					char* buffer2 = malloc(1000);
					read(listfd, buffer2, 949); // fixed size
					sprintf(buffer, buffer2, clientHeader.filename, miniBuffer);
					free(buffer2);
					free(miniBuffer);
				}
				
				safeSend(sessionSock, buffer, strlen(buffer));
			}
		}
	}

	close(sessionSock);
}

int main(int argc, char** argv) {
	setupManager(); // for user logging

	int port = atoi(argv[1]);
	if(argc == 3) {
		DOCUMENT_ROOT = argv[2];
		int len = strlen(DOCUMENT_ROOT);
		if(DOCUMENT_ROOT[len - 1] == '/') {
			DOCUMENT_ROOT[len - 1] = '\0'; // no end slash
		}
	}
	if(strstr(DOCUMENT_ROOT, "WWW2") == 0) { // we are using the boring dir
		printf("I see you are using root directory '%s' why not use 'WWW2/' instead? This webserver has a lot more features than required, please run it on 'WWW2' to see them all :)\n", DOCUMENT_ROOT);
	} 
	int mainSock = socket(AF_INET6, SOCK_STREAM, 0);
	if(mainSock < 0) {
		printf("Setup failed\n");
		exit(1);
	}
	int reuse_true = 1;

	// no timeout between restarting servers
	setsockopt(mainSock, SOL_SOCKET, SO_REUSEADDR, &reuse_true, sizeof(reuse_true));

	struct sockaddr_in6 addr; // internet socket address data structure
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port); // byte order is significant
	addr.sin6_addr = in6addr_any; // listen to all interfaces

	if(bind(mainSock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Setup failed\n");
		exit(1);
	}

	if(listen(mainSock, ON_HOLD_COUNT) < 0) {
		printf("Setup failed\n");
		exit(1);
	}

	int sessionSock;
	struct sockaddr_in client_addr;
	unsigned int socklen = sizeof(client_addr);
	User* client;

	while(serverRunning) {
		if((sessionSock = accept(mainSock, (struct sockaddr*) &client_addr, &socklen)) > 0) {
			client = getUser((char*) &client_addr.sin_addr);
			if(userMaxedResources(client)) {
				close(sessionSock);
				printf("%i has maxed out resources!\n", (int*) &client_addr.sin_addr);
			}
			else {
				handelRequest(sessionSock, process, client); // spans thread -> process
			}
		}
	}
	close(mainSock);

	return 0;
}