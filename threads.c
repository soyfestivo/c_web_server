#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads.h"

typedef struct thread_struct {
	volatile pthread_t threadId;
	volatile void (*functionCall) (int, void*); 
	volatile int sessionSock;
	volatile void* ptr;
} thread_struct;

volatile thread_struct threads[THREAD_COUNT];

volatile int threadsInUse = 0; // oh snap, hope this doesn't lead to issues

void serverRequestDaemon(thread_struct* launchInfo) {
	launchInfo->functionCall(launchInfo->sessionSock, launchInfo->ptr);

	//printf("<---done with call %i\n", launchInfo->threadId);
	launchInfo->threadId = 0; // we're done
	//printf("Some thread closes, %i remain\n", threadsInUse);

	pthread_exit(NULL);
}

void findAndMakeThread(int sessionSock, void (*f) (int, void*), void* ptr) {
	int i;
	for(i = 0; i < THREAD_COUNT; i++) {
		if(threads[i].threadId == 0) { // we found a winner
			threads[i].sessionSock = sessionSock;
			threads[i].functionCall = f;
			threads[i].ptr = ptr;
			//printf("making new thread! sock: %i\n", sessionSock);
			if(pthread_create(&(threads[i].threadId), NULL, serverRequestDaemon, (void *) &threads[i]) == 0) {
				return;
			}
			else { // some sort of error?...
				printf("ERROR: out of threads\n");
				exit(-1);
			}
		}
	}
}

int countThreads() {
	int c = 0;
	int i;
	for(i = 0; i < THREAD_COUNT; i++) {
		if(threads[i].threadId != 0) {
			c++;
		}
	}
	return c;
}

void printThreads() {
	int i;
	for(i = 0; i < THREAD_COUNT; i++) {
		printf("t:%i,%i\n", i, threads[i].threadId);
	}
}

void printFunction(void (*f) (int)) {
	char* c = (char*) f;
	while(*c != 0xc3) { // retq
		printf("%x", *c);
		c++;
	}
}

void handelRequest(int sessionSock, void (*f) (int, void*), void* ptr) {
	//printFunction(f);
	//exit(0);
	if(countThreads() < THREAD_COUNT) {
		findAndMakeThread(sessionSock, f, ptr);
	}
	else {
		while(countThreads() >= THREAD_COUNT) {
			sleep(150); // and now we wait for an opening
		}
		findAndMakeThread(sessionSock, f, ptr); // one opened up!
	}
	//f(sessionSock); // test
}