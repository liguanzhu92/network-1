# Makefile for client and server

CC = gcc
OBJSRV = server.c tcp_daemon/tcpdio.c tcp_daemon/headers/tcpd.h
OBJCLI = client.c tcp_daemon/tcpdio.c tcp_daemon/headers/tcpd.h
OBJTCPD = tcp_daemon/tcpd.c tcp_daemon/headers/tcpd.h tcp_daemon/headers/troll.h tcp_daemon/headers/check_sum.h
OBJTIMER = tcp_daemon/timer.c tcp_daemon/delta_list.c tcp_daemon/headers/delta_list.h tcp_daemon/headers/tcpd.h
OBJTIMER_TEST = tcp_daemon/timer_test.c tcp_daemon/headers/delta_list.h

OUT_DIR = bin/
MK_DIR = mkdir -p

CFLAGS =
# setup for system
LIBS =

all: $(OUT_DIR) ftps ftpc tcpd timer timer_test

$(OUT_DIR):
	$(MK_DIR) $(OUT_DIR)
ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $(OUT_DIR)$@ $(OBJSRV) $(LIBS)
ftpc:   $(OBJCLI)
	$(CC) $(CFLAGS) -o $(OUT_DIR)$@ $(OBJCLI) $(LIBS)
tcpd:   $(OBJTCPD)
	$(CC) $(CFLAGS) -o $(OUT_DIR)$@ $(OBJTCPD) $(LIBS)
timer:  $(OBJTIMER)
	$(CC) $(CFLAGS) -o $(OUT_DIR)$@ $(OBJTIMER) $(LIBS)
timer_test: $(OBJTIMER_TEST)
	$(CC) $(CFLAGS) -o $(OUT_DIR)$@ $(OBJTIMER_TEST) $(LIBS)

clean:
	rm -rf recv/ `find bin/* -name "*" ! -name "troll"`
