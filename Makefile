CC = g++
CFLAGS = -g

all: client server

clientsocket: client.c
	$(CC) $(CFLAGS) -lpthread client.c -o client
serversocket: server.c
	$(CC) $(CFLAGS) -lpthread server.c -o server
clean:
	rm -f client.o server.o
