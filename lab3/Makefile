# CODED BY GUANZHU Li (li.5328) & JiABEi XU (xu.1717)
# Makefile for client and server

CC = gcc
OBJSRV = client.c tcpdio.c ftp.h tcpd.h
OBJCLI = server.c tcpdio.c ftp.h tcpd.h
OBJTCPD = tcpd.c tcpd.h troll.h
CFLAGS =
# setup for system
LIBS =

all: ftps ftpc tcpd

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)
ftpc:	$(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)
tcpd:   $(OBJTCPD)
	$(CC) $(CFLAGS) -o $@ $(OBJTCPD) $(LIBS)

clean:
	rm ftps ftpc tcpd
