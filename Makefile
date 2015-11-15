TARGETS=homework4 *.o *.gch

CFLAGS=-Wall -g -O0

all: $(TARGETS)
clean:
	rm -f $(TARGETS)
threads.o: threads.h threads.c
	gcc -c threads.c threads.h
protocols.o: protocols.h protocols.c
	gcc -c protocols.c protocols.h
manager.o: manager.h manager.c
	gcc -c manager.c manager.h
web_server: web_server.c threads.o protocols.o manager.o
	gcc $(CFLAGS) -pthread -o web_server web_server.c threads.o protocols.o manager.o
	rm *.o *.gch