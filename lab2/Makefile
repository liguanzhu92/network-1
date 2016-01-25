# Makefile for client and server

CC = gcc
OBJSRV = server.c
CFLAGS =
# setup for system
LIBS =

all: ftps

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)

clean:
	rm ftps