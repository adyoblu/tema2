CC = g++
CFLAGS = -g

all: client server

client: client.c file.c
	$(CC) $(CFLAGS) client.c file.c -o client

server: server.c file.c
	$(CC) $(CFLAGS) -lpthread server.c file.c -o server

clean:
	rm -f client server file
