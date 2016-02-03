// CODED BY GUANZHU Li (li.5328) & JiABEi XU (xu.1717) 
/* server.c using TCP */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include "ftp.h"
#include "tcpd.h"

/* server program called with no argument */
int main(int argc, char **argv) {
    int                sock;                           /* initial socket descriptor */
    int                tcpd_sock;                                     /* each client connection has a
                                                         * unique socket descriptor */
    char               buf_in[BUFFER_SIZE];
    struct sockaddr_in sin_addr;                       /* structure for server socket addr */
    struct sockaddr_in cin_addr;                        /* structure for client socket addr */
    struct sockaddr_in tcpd_addr;
    int                addr_len;
    struct in_addr     cip_addr;                        /* structure for client ip address */
    FILE               *fp;
    unsigned long      file_size            = 0;
    char               file_name[FILE_NAME_LENGTH];

    if (argc != 3) {
        printf("Usage : ftps <local-port> <tcpd-port>");
        exit(1);
    }

    printf("TCP server waiting for remote connection from clients ...\n");

    /*initialize socket connection in unix domain*/
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error opening datagram socket");
        exit(1);
    }

    tcpd_sock = sock;

    /* construct name of socket to send to */
    sin_addr.sin_family      = AF_INET;
    sin_addr.sin_addr.s_addr = INADDR_ANY;
    sin_addr.sin_port        = htons(atoi(argv[1]));

    tcpd_addr.sin_family     = AF_INET;
    tcpd_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tcpd_addr.sin_port       = htons(atoi(argv[2]));

    /* bind socket name to socket */
    if (bind(sock, (struct sockaddr *) &sin_addr, sizeof(struct sockaddr)) < 0) {
        perror("error binding stream socket");
        exit(1);
    }

    /* send hello msg to client */
    /*if (send(msgsock, buf_out, strlen(buf_out) + 1, 0) < 0) {
        perror("Error sending message to client");
        exit(1);
    }*/

    printf("Registering with TCPD...\n");

    //Registering with TCPD and sending port number for receiving datagrams
    if(SEND(tcpd_sock, argv[1], sizeof(argv[1]), 0) < 0) {
        perror("Error sending datagram message");
        exit(1);
    }

    printf("Registered, waiting for datagrams...\n");

    /* get file size */
    bzero(buf_in, BUFFER_SIZE);
    if (RECV(sock, buf_in, FILE_SIZE_LENGTH, 0) < 0) {
        perror("Error receiving message from client");
        exit(1);
    }
    memcpy(&file_size, buf_in, FILE_SIZE_LENGTH);
    file_size = ntohl(file_size);
    bzero(buf_in, FILE_SIZE_LENGTH);

    /* get file name */
    if (RECV(sock, buf_in, FILE_NAME_LENGTH, MSG_WAITALL) < 0) {
        perror("Error receiving message from client");
        exit(1);
    }
    strncpy(file_name, buf_in, FILE_NAME_LENGTH);
    printf("File name: %s, size: %ld\n", file_name, file_size);
    bzero(buf_in, FILE_NAME_LENGTH);

    /* receive file */
    fp = fopen(file_name, "w+");
    if (fp == NULL) {
        perror("Error");
        exit(1);
    }
    int current_len = 0;
    while ((current_len = RECV(sock, buf_in, BUFFER_SIZE, 0)) > 0) {
        fwrite(buf_in, sizeof(char), current_len, fp);
    }
    /*if(current_len < 0) {
        perror("Error transferring file from client");
        exit(1);
    }*/
    fclose(fp);

    /* print confirmation msg */
    memcpy(&cip_addr, &cin_addr.sin_addr, IP_ADDR_LENGTH);
    printf("Received %s from client %s: %d\n", file_name, inet_ntoa(cip_addr), ntohs(cin_addr.sin_port));

    /* close all connections and remove socket file */
    close(sock);

    return 0;
}
