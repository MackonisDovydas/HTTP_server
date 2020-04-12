CC = g++
DEBUG = -g
CFLAGS = -Wall -lpthread -c $(DEBUG)
LFLAGS = -Wall -lpthread $(DEBUG)

all: client server

client: client.o
ifeq ($(OS),Windows_NT)
	$(CC) $(LFLAGS) client.o -lws2_32 -o client
else 
	$(CC) $(LFLAGS) client.o -o client
endif

server: server.o
ifeq ($(OS),Windows_NT)
	$(CC) $(LFLAGS) server.o -lws2_32 -o server
else
	$(CC) $(LFLAGS) server.o -o server
endif

client.o: client.cpp HTTP_utils.h
	$(CC) $(CFLAGS) client.cpp

server.o: server.cpp HTTP_utils.h
	$(CC) $(CFLAGS) server.cpp

clean:
	rm -rf *.o *~ client server
