# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: echoServer echoClient

csapp.o: ../csapp.c csapp.h
	$(CC) $(CFLAGS) -c ../csapp.c

echoServer.o: echoServer.c csapp.h
	$(CC) $(CFLAGS) -c echoServer.c 

echoClient.o: echoClient.c csapp.h
	$(CC) $(CFLAGS) -c echoClient.c

echoServer: echoServer.o csapp.o
	$(CC) $(CFLAGS) -o $@ echoServer.o csapp.o

echoClient: echoClient.o csapp.o
	$(CC) $(CFLAGS) -o $@ echoClient.o csapp.o

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o echoClient echoServer