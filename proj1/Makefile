# CODED BY GUANZHU Li (li.5328) & JiABEi XU (xu.1717)
# Makefile for client and server

CC = gcc
OBJSRV = server.c tcpdio.c ftp.h tcpd.h
OBJCLI = client.c tcpdio.c ftp.h tcpd.h
OBJTCPD = tcpd.c tcpd.h troll.h check_sum.h
OBJTIMER = timer.c delta_list.c delta_list.h
OBJTIMER_TEST = timer_test.c delta_list.h

CFLAGS =
# setup for system
LIBS =

all: ftps ftpc tcpd timer timer_test

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)
ftpc:   $(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)
tcpd:   $(OBJTCPD)
	$(CC) $(CFLAGS) -o $@ $(OBJTCPD) $(LIBS)
timer:  $(OBJTIMER)
	$(CC) $(CFLAGS) -o $@ $(OBJTIMER) $(LIBS)
timer_test: $(OBJTIMER_TEST)
	$(CC) $(CFLAGS) -o $@ $(OBJTIMER_TEST) $(LIBS)

clean:
	rm -rf recv/ ftps ftpc tcpd timer timer_test
