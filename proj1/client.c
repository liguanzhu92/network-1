// CODED BY GUANZHU Li (li.5328) & JiABEi XU (xu.1717)
/* client.c using TCP */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include "ftp.h"
#include "tcpd.h"
#include "check_sum.h"

/* client program called with host name where server is run */
int main(int argc, char **argv) {
    int                sock;                    /* initial socket descriptor */
    struct sockaddr_in sin_addr;                /* structure for socket name setup */
    FILE               *fp;                     /* file sent to server */
    unsigned long      file_size  = 0;          /* initialize file size */
    const char         *HOST_NAME = argv[1];    /* host name */
    const char         *PORT      = argv[2];    /* port number */
    const char         *FILE_NAME = argv[3];    /* file name */
    struct in_addr     sip_addr;                /* structure for server ip address */
    struct hostent     *hp;                     /* structure host information */
    struct stat        st;                      /* structure file information */
    struct TcpdMessage message;

    /* Improper usage */
    if (argc != 4) {
        printf("Usage : ftpc <remote-IP> <remote-port> <local-file-to-transfer>");
        exit(1);
    }

    /* initialize socket connection in unix domain */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error opening datagram socket");
        exit(1);
    }

    if (PORT == 0) {
        fprintf(stderr, "%s: unknown host\n", argv[1]);
        exit(2);
    }

    /* construct name of socket to send to */
    hp = gethostbyname(HOST_NAME);
    if (hp == 0) {
        fprintf(stderr, "%s: unknown host\n", argv[1]);
        exit(2);
    }
    bcopy((void *) hp->h_addr, (void *) &sin_addr.sin_addr, hp->h_length);
    sin_addr.sin_family = htons(AF_INET);
    sin_addr.sin_port   = htons(atoi(PORT));


    if (CONNECT(sock, (struct sockaddr *) &sin_addr, sizeof(struct sockaddr_in)) < 0) {
        close(sock);
        perror("error connecting stream socket");
        exit(1);
    }

    message.header = sin_addr;

    /* send file size */
    fp = fopen(FILE_NAME, "rb");
    if (fp == NULL) {
        perror("Error");
        exit(-1);
    }
    /* calculate the file size */
    if (stat(FILE_NAME, &st) >= 0) {
        file_size = htonl(st.st_size);
    }
    bzero(message.contents, BUFFER_SIZE);
    memcpy(message.contents, &file_size, FILE_SIZE_LENGTH);
    message.check_sum = htons(cal_crc(message.contents, FILE_SIZE_LENGTH));
    //printf("file size:%hu\n", cal_crc(message.contents, FILE_SIZE_LENGTH));

    if (SEND(sock, (char *) &message, FILE_SIZE_LENGTH + HEADER_LENTH + sizeof(unsigned short), 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }
    puts(message.contents);
    bzero(message.contents, FILE_SIZE_LENGTH);

    /* send file name */
    strncpy(message.contents, FILE_NAME, strlen(FILE_NAME));
    message.check_sum = htons(cal_crc(message.contents, FILE_NAME_LENGTH));
    //printf("file name:%hu\n", ntohs(message.check_sum));

    if (SEND(sock, (char *) &message, FILE_NAME_LENGTH + HEADER_LENTH + sizeof(unsigned short), 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }
    printf("File name: %s, size: %d\n", message.contents, ntohl(file_size));
    bzero(message.contents, FILE_NAME_LENGTH);

    /* send file */
    int current_len = 0;
    while ((current_len = fread(message.contents, 1, BUFFER_SIZE, fp)) > 0) {
        message.check_sum = htons(cal_crc(message.contents, current_len));
        //printf("file content:%hu\n", ntohs(message.check_sum));
        SEND(sock, (char *) &message, current_len + HEADER_LENTH + sizeof(unsigned short), 0);
        usleep(10000);
    }

    fclose(fp);

    /* print confirmation msg */
    memcpy(&sip_addr, &sin_addr.sin_addr, IP_ADDR_LENGTH);
    printf("Sent %s to server %s: %s\n", FILE_NAME, inet_ntoa(sip_addr), PORT);

    /* close all connections and remove socket file */
    close(sock);
    return 0;
}
