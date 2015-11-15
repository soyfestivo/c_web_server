#include <sys/stat.h>
#include <regex.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "protocols.h"

char* DOCUMENT_ROOT = "./WWW2";

char* STATUS_OK = "HTTP/1.0 200 OK\r\n";
char* STATUS_REDIRECT = "HTTP/1.0 302 Found\r\n";
char* STATUS_FORBIDDEN = "HTTP/1.0 403 Forbidden\r\n";
char* STATUS_NOT_FOUND = "HTTP/1.0 404 Not Found\r\n";
char* HEADER_SERVER = "Server: Stephen's C webserver\r\n";
char* HEADER_KEEP_ALIVE = "Connection:keep-alive\r\n";
char* HEADER_END = "\r\n";

char* CONTENT_TYPE_TEXT = "Content-type: text/plain; charset=UTF-8\r\n";
char* CONTENT_TYPE_HTML = "Content-type: text/html; charset=UTF-8\r\n";
char* CONTENT_TYPE_CSS = "Content-type: text/css; charset=UTF-8\r\n";
char* CONTENT_TYPE_EXE = "Content-type: application/octet-stream\r\n";
char* CONTENT_TYPE_DOC = "Content-type: application/msword\r\n";

char* CONTENT_TYPE_BMP = "Content-type: image/bmp\r\n";
char* CONTENT_TYPE_PNG = "Content-type: image/png\r\n";
char* CONTENT_TYPE_GIF = "Content-type: image/gif\r\n";
char* CONTENT_TYPE_JPG = "Content-type: image/jpg\r\n";
char* CONTENT_TYPE_JPEG = "Content-type: image/jpeg\r\n";
char* CONTENT_TYPE_ICO = "Content-type: image/x-icon\r\n";

char* CONTENT_TYPE_JS = "Content-type: application/x-javascript\r\n";
char* CONTENT_TYPE_PDF = "Content-type: application/pdf\r\n";
char* CONTENT_TYPE_MP3 = "Content-type: audio/mpeg\r\n";

char* getContentType(Header* h) {
	struct stat fileMeta;
	if(stat(h->filename, &fileMeta) != 0) {
		return CONTENT_TYPE_HTML; // this is for 404
	}
	if(strcmp("html", h->fileType) == 0 || strcmp("php", h->fileType) == 0) {
		return CONTENT_TYPE_HTML;
	}
	if(strcmp("css", h->fileType) == 0) {
		return CONTENT_TYPE_CSS;
	}
	if(strcmp("js", h->fileType) == 0) {
		return CONTENT_TYPE_JS;
	}
	if(strcmp("jpg", h->fileType) == 0) {
		return CONTENT_TYPE_JPG;
	}
	if(strcmp("jpeg", h->fileType) == 0) {
		return CONTENT_TYPE_JPEG;
	}
	if(strcmp("png", h->fileType) == 0) {
		return CONTENT_TYPE_PNG;
	}
	if(strcmp("gif", h->fileType) == 0) {
		return CONTENT_TYPE_GIF;
	}
	if(strcmp("bmp", h->fileType) == 0) {
		return CONTENT_TYPE_BMP;
	}
	if(strcmp("pdf", h->fileType) == 0) {
		return CONTENT_TYPE_PDF;
	}
	if(strcmp("exe", h->fileType) == 0 || strcmp("zip", h->fileType) == 0) {
		return CONTENT_TYPE_EXE;
	}
	if(strcmp("doc", h->fileType) == 0 || strcmp("docx", h->fileType) == 0) {
		return CONTENT_TYPE_DOC;
	}
	if(strcmp("mp3", h->fileType) == 0) {
		return CONTENT_TYPE_MP3;
	}
	if(strcmp("ico", h->fileType) == 0) {
		return CONTENT_TYPE_ICO;
	}
	return CONTENT_TYPE_HTML; // default
}

void processHeader(char data[], Header* returnStruct) {
	regex_t commandR;
	regmatch_t match[2];

	// decide get or post
	if(data[0] == 'G') { // get
		returnStruct->type = GET;
	}
	if(data[0] == 'P') { // post
		returnStruct->type = POST;
	}

	// decide requested path
	regcomp(&commandR, " (.*) HTTP/1.*", REG_ICASE|REG_EXTENDED);
	regexec(&commandR, data, 2, match, 0);
	int pathLength = match[1].rm_eo - match[1].rm_so;
	memset(returnStruct->filename, 0, 4096);
	memcpy(returnStruct->filename, &(data[match[1].rm_so]), pathLength);

	// decide file type
	regcomp(&commandR, "\\.([a-z0-9]+)", REG_ICASE|REG_EXTENDED);
	if(regexec(&commandR, returnStruct->filename, 2, match, 0) != REG_NOMATCH) { // has file type
		memset(returnStruct->fileType, 0, 16);
		pathLength = match[1].rm_eo - match[1].rm_so;
		memcpy(returnStruct->fileType, &(returnStruct->filename[match[1].rm_so]), pathLength);
	}

	// decide content length if post
	regcomp(&commandR, "Content-Length: ([0-9]*)", REG_ICASE|REG_EXTENDED);
	if(returnStruct->type == POST && regexec(&commandR, data, 2, match, 0) != REG_NOMATCH) {
		char number[32];
		memset(number, 0, 32);
		pathLength = match[1].rm_eo - match[1].rm_so;
		memcpy(number, &(data[match[1].rm_so]), pathLength);
		returnStruct->contentLength = atoi(number);
	}
	else {
		returnStruct->contentLength = 0;
	}
}

void safeSend(int socketFd, void* data, int n) {
	int written = 0;
	int count = 0; // make sure we don't try too much
	while((written += send(socketFd, &(data[written]), n - written, 0)) < n && written > -1 && count < 200) {
		if(errno != 0) { // EPIPE == broken pipe :(
			return;
		}
		count++;
	}
}