#ifndef _NET_H
#define _NET_H
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#endif
#define TCPD_PORT_S 3870
#define TCPD_PORT 3860
#define TROLL_PORT 3850
#define MAXBUF 1000
typedef struct TCPD_MSG {
    struct sockaddr_in header;
    char contents[MAXBUF];
} TCPD_MSG;

int SEND(int socket, const void *buffer, size_t len, int flags);
int RECV(int socket, void *buffer, size_t length, int flags);
void tcpd_server(int argc, char **argv);
void tcpd_client(int argc, char **argv);
