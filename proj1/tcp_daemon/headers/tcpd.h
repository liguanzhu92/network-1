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
#define TCPD_PORT_C 7100
#define TROLL_PORT 7000
#define CTRL_PORT 3900
#define TIMER_PORT_CLIENT 3880
#define TIMER_PORT_SERVER 3890
#define ACK_PORT_C 7010
#define ACK_PORT_S 3860

#define FILE_SIZE_LENGTH 4
#define FILE_NAME_LENGTH 20
#define IP_ADDR_LENGTH 4
#define OPENED_CONNECTIONS 5

#define TCPD_HEADER_LENGTH 16
#define TCP_HEADER_LENGTH 20
#define CONTENT_BUFF_SIZE 1000
#define TCPD_MESSAGE_SIZE TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH + CONTENT_BUFF_SIZE
#define SOCK_BUF_SIZE (128*FD_SETSIZE)

#define WINDOW_SIZE 20
#define TCPD_BUF_SIZE 64

typedef struct TcpdMessage {
    struct sockaddr_in tcpd_header;
    struct tcphdr tcp_header;
    char   contents[CONTENT_BUFF_SIZE];
} TcpdMessage;

int  SEND(int socket, const void *buffer, size_t len, int flags);
int  RECV(int socket, void *buffer, size_t length, int flags);
int  CONNECT(int socket, const struct sockaddr *address, socklen_t address_len);
int  ACCEPT(int socket, void *address, socklen_t *address_len);
int  BIND(int socket, const struct sockaddr *address, socklen_t address_len);
int  RECV_CTRL(int socket, void *buffer, size_t len, int flags);
void tcpd_server();
void tcpd_client();
void __init_client_sock_c(int *client_sock, struct sockaddr_in *client_addr);
void __init_ctrl_sock_c(int *ctrl_sock, struct sockaddr_in *ctrl_addr);
void __init_ack_sock_c(int *ack_sock, struct sockaddr_in *ack_addr, int new_buff);
void __init_timer_send_sock_c(int *timer_send_sock, struct sockaddr_in *timer_send_addr);
void __init_timer_recv_sock_c(int *timer_recv_sock, struct sockaddr_in *timer_recv_addr, int new_buff);
void __init_troll_sock_c(int *troll_sock, struct sockaddr_in *troll_addr);
int is_window_empty(int window[]);

#endif
