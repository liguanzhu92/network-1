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
#define LOCAL_HOST "127.0.0.1"
#define TCPD_PORT_S 3870
#define TCPD_PORT 7100
#define TROLL_PORT 7000
#define MAXBUF 1000
#define HEADER_LENTH 16
typedef struct TCPD_MSG {
    struct sockaddr_in header;
    char contents[MAXBUF];
} tcpd_msg;

int SEND(int socket, const void *buffer, size_t len, int flags);
int RECV(int socket, void *buffer, size_t length, int flags);
int CONNECT(int socket, const struct sockaddr *address, socklen_t address_len);
int ACCEPT(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);
void tcpd_server();
void tcpd_client();
