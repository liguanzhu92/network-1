# CODED BY GUANZHU Li (li.5328) & JiABEi XU (xu.1717)
# Makefile for client and server

CC = gcc
OBJSRV = server.c
OBJCLI = client.c
CFLAGS =
# setup for system
LIBS =

all: ftps ftpc

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)
ftpc:	$(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)

clean:
	rm ftps ftpc
