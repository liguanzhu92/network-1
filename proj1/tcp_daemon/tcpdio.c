#include "headers/tcpd.h"

int SEND(int socket, const void *buffer, size_t len, int flags) {
    struct sockaddr_in srv_addr;
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_port        = htons(TCPD_PORT_C);
    srv_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    int ret;
    if ((ret = sendto(socket, buffer, len, flags, (struct sockaddr *) &srv_addr, sizeof(struct sockaddr_in))) < 0) {
        perror("send error");
    }
    return ret;
}

int RECV(int socket, void *buffer, size_t len, int flags) {
    struct sockaddr_in srv_addr;
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_port        = htons(TCPD_PORT_S);
    srv_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    int srv_addr_length = sizeof(struct sockaddr_in);
    int ret;
    if ((ret = recvfrom(socket, buffer, len, flags, (struct sockaddr *) &srv_addr, &srv_addr_length)) < 0) {
        perror("recv error");
    }
    return ret;
}

int CONNECT(int socket, const struct sockaddr *address, socklen_t address_len) {
    return 0;
}

int ACCEPT(int socket, void *address, socklen_t *address_len) {
    return 0;
}

int BIND(int socket, const struct sockaddr *address, socklen_t address_len) {
    return bind(socket, address, address_len);
}

int RECV_CTRL(int socket, void *buffer, size_t len, int flags) {
    struct sockaddr_in srv_addr;
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_port        = htons(CTRL_PORT);
    srv_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    int srv_addr_length = sizeof(struct sockaddr_in);
    int ret;
    if ((ret = recvfrom(socket, buffer, len, flags, (struct sockaddr *) &srv_addr, &srv_addr_length)) < 0) {
        perror("recv error");
    }
    return ret;
}

int CLOSE(int sock) {
    return close(sock);
}