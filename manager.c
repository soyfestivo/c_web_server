#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include "manager.h"

#define MAX_TRANSFERS 2147483648 // 2GB
#define MAX_REQUESTS 18000   // 18k

User* list = NULL;
time_t lastTime;

void setupManager() {
	time(&lastTime);
}

void removeUser(User* user) {
	User* tmp = list;
	User* tmp2;
	if(tmp == NULL) {
		return;
	}
	while(tmp->next != NULL && memcmp(tmp->next->ip, user->ip, 16) != 0) {
		tmp = tmp->next;
	}
	if(tmp->next != NULL && memcmp(tmp->next->ip, user->ip, 16) == 0) { // we found the right one
		tmp2 = tmp->next;
		tmp->next = tmp2->next;
		free(tmp2);
	}
}

int authenticate(User* user, char* data) {
	regex_t commandR;
	regmatch_t match[3];
	regcomp(&commandR, "username=([a-z0-9]+)&password=([a-z0-9]+)", REG_ICASE|REG_EXTENDED);
	if(regexec(&commandR, data, 3, match, 0) != 0) {
		printf("error?\n");
	}
	char username[64];
	char password[64];
	memset(username, 0, 64);
	memset(password, 0, 64);
	printf("%i\n", match[1].rm_eo);
	printf("%i\n", match[2].rm_eo);
	memcpy(&username, &data[match[1].rm_so], match[1].rm_eo - match[1].rm_so);
	memcpy(&password, &data[match[2].rm_so], match[2].rm_eo - match[2].rm_so);

	if((strcmp(username, "sselke2") == 0 && strcmp(password, "password") == 0) ||
	   (strcmp(username, "vsrini7") == 0 && strcmp(password, "password") == 0) ||
	   (strcmp(username,  "zshi22") == 0 && strcmp(password, "password") == 0) ||
	   (strcmp(username,"cynthiat") == 0 && strcmp(password, "password") == 0)) {
	   	user->isAuthorized = 1;
		return 1;
	}
	else {
		return 0;
	}
}

int signout(User* user) {
	user->isAuthorized = 0;
}

int isAuthenticated(User* user) {
	return user->isAuthorized;
}

void checkTimeDifference() {
	time_t now;
	time(&now);

	if(difftime(now, lastTime) > 3600.0) { // more than 1 hour has elapsed
		User* tmp = list;
		User* tmp2;
		while(tmp != NULL) {
			if(difftime(tmp->timestamp, now) > 10800.0) { // has been more than 3 hours since last connect
				tmp2 = tmp;
				tmp = tmp->next;
				removeUser(tmp2); // TODO
			}
			else {

				tmp = tmp->next;
			}
		}
	}
}

User* newUser(User** last, char* ip) {
	User* user = malloc(sizeof(User));
	memcpy(user->ip, ip, 16);
	user->transfers = 0;
	user->requests = 0;
	user->isAuthorized = 0;
	time(&user->timestamp);
	user->username[0] = '\0';
	user->next = NULL;
	*last = user;
	return user;
}

// adds request
User* getUser(char* ip) {
	checkTimeDifference();
	User* tmp = list;
	if(tmp == NULL) { // list is empty
		return newUser(&list, ip);
	}
	while(tmp->next != NULL) {
		if(memcmp(tmp->ip, ip, 16) == 0) { // same user
			time(&tmp->timestamp);
			tmp->requests++;
			return tmp;
		}
		tmp = tmp->next;
	}
	// end of list
	if(memcmp(tmp->ip, ip, 16) == 0) { // same user
		time(&tmp->timestamp);
		tmp->requests++;
		return tmp;
	}
	// user does not exist, add to end
	return newUser(&tmp->next, ip);
}

int userMaxedResources(User* user) {
	return (user->transfers > MAX_TRANSFERS || user->requests > MAX_REQUESTS);
}