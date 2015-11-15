#include <time.h>

typedef struct user {
	unsigned char ip[16];
	unsigned int transfers;
	unsigned int requests;
	time_t timestamp;
	char username[16];
	int isAuthorized;
	struct user* next;
} User;

extern void setupManager();
extern User* getUser(char* ip);
extern int userMaxedResources(User* user);
extern int authenticate(User* user, char* data);
extern int isAuthenticated(User* user);
extern int signout(User* user);