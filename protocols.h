#define GET 0
#define POST 1

extern char* DOCUMENT_ROOT;

extern char* STATUS_OK;
extern char* STATUS_REDIRECT;
extern char* STATUS_FORBIDDEN;
extern char* STATUS_NOT_FOUND;
extern char* HEADER_SERVER;
extern char* HEADER_END;
extern char* HEADER_KEEP_ALIVE;

extern char* CONTENT_TYPE_TEXT;
extern char* CONTENT_TYPE_HTML;
extern char* CONTENT_TYPE_CSS;
extern char* CONTENT_TYPE_EXE;
extern char* CONTENT_TYPE_DOC;

extern char* CONTENT_TYPE_BMP;
extern char* CONTENT_TYPE_PNG;
extern char* CONTENT_TYPE_GIF;
extern char* CONTENT_TYPE_JPG;
extern char* CONTENT_TYPE_JPEG;
extern char* CONTENT_TYPE_ICO;

extern char* CONTENT_TYPE_JS;
extern char* CONTENT_TYPE_PDF;
extern char* CONTENT_TYPE_MP3;

typedef struct header {
	short type;
	char filename[4096];
	char fileType[16];
	int contentLength;
} Header;

extern char* getContentType(Header* h);
extern void processHeader(char data[], Header* returnStruct);
extern void safeSend(int socketFd, void* data, int n);
