#ifndef TCPD_H
#define TCPD_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define LOCAL_HOST "127.0.0.1"
#define TCPD_PORT_S 3870
#define TCPD_PORT 7100
#define TROLL_PORT 7001
#define TIMER_PORT_CLIENT 3880
#define TIMER_PORT_SERVER 3890

#define FILE_SIZE_LENGTH 4
#define FILE_NAME_LENGTH 20
#define IP_ADDR_LENGTH 4
#define OPENED_CONNECTIONS 5

#define TCPD_HEADER_LENGTH 16
#define TCP_HEADER_LENGTH 20
#define CONTENT_BUFF_SIZE 1000
#define SOCK_BUF_SIZE 128*1024

typedef struct TcpdMessage {
    struct sockaddr_in header;
    struct tcphdr tcp_header;
    char   contents[CONTENT_BUFF_SIZE];
} TcpdMessage;

int  SEND(int socket, const void *buffer, size_t len, int flags);

int  RECV(int socket, void *buffer, size_t length, int flags);

int  CONNECT(int socket, const struct sockaddr *address, socklen_t address_len);

int  ACCEPT(int socket, void *address, socklen_t *address_len);

void tcpd_server();

void tcpd_client();

#endif
