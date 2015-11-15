#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>


int main(int argc, char** argv) {

	int port = atoi(argv[1]);
	int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
	if(server_sock < 0) {
		printf("Setup failed\n");
		exit(1);
	}
	int reuse_true = 1;

	// no timeout between restarting servers
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true, sizeof(reuse_true));

    

	return 0;
}