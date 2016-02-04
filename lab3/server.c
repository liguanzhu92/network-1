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
    int                sock;                            /* initial socket descriptor */
    int                msgsock;
    int                addr_len;
    char               buf_in[BUFFER_SIZE];
    struct sockaddr_in sin_addr;                        /* structure for server socket addr */
    struct sockaddr_in cin_addr;                        /* structure for client socket addr */
    struct in_addr     cip_addr;                        /* structure for client ip address */
    FILE               *fp;
    unsigned long      file_size = 0;
    char               file_name[FILE_NAME_LENGTH];

    if (argc != 2) {
        printf("Usage : ftps <local-port>");
        exit(1);
    }

    printf("TCP server waiting for remote connection from clients ...\n");

    /*initialize socket connection in unix domain*/
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error opening datagram socket");
        exit(1);
    }

    /* construct name of socket to send to */
    sin_addr.sin_family      = AF_INET;
    sin_addr.sin_addr.s_addr = INADDR_ANY;
    sin_addr.sin_port        = htons(atoi(argv[1]));

    /* bind socket name to socket */
    if (bind(sock, (struct sockaddr *) &sin_addr, sizeof(struct sockaddr)) < 0) {
        perror("error binding stream socket");
        exit(1);
    }

    /* listen for socket connection and set max opened socket connections to 5 */
    listen(sock, OPENED_CONNECTIONS);

    /* accept a (1) connection in socket msgsocket */
    addr_len     = sizeof(cin_addr);
    if ((msgsock = ACCEPT(sock, (struct sockaddr *) &cin_addr, &addr_len)) == -1) {
        perror("Error connecting stream socket");
        exit(1);
    }

    /* get file size */
    bzero(buf_in, BUFFER_SIZE);
    if (RECV(sock, buf_in, FILE_SIZE_LENGTH, MSG_WAITALL) < 0) {
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
    char dest[30] = "recv/";
    strcat(dest, file_name);
    fp = fopen(dest, "w+");
    if (fp == NULL) {
        perror("Error");
        exit(1);
    }
    int current_len = 0;
    int count       = 0;
    while ((current_len = RECV(sock, buf_in, BUFFER_SIZE, 0)) > 0) {
        count += current_len;
        fwrite(buf_in, sizeof(char), current_len, fp);
        //fprintf(stderr, "%d", count);
        if (count == file_size) {
            break;
        }
    }
    fclose(fp);

    /* print confirmation msg */
    memcpy(&cip_addr, &cin_addr.sin_addr, IP_ADDR_LENGTH);
    printf("Received %s from client %s: %d\n", file_name, inet_ntoa(cip_addr), ntohs(cin_addr.sin_port));

    /* close all connections and remove socket file */
    close(msgsock);
    close(sock);

    return 0;
}
