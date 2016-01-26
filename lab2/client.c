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

/* client program called with host name where server is run */
int main(int argc, char *argv[]) {
    int                sock;                       /* initial socket descriptor */
    struct sockaddr_in sin_addr;       /* structure for socket name setup */
    char               buf_in[BUFFER_SIZE];
    char               buf_out[BUFFER_SIZE];       /* message to set to server */
    FILE               *fp;
    unsigned long      file_size  = 0;
    const char         *HOST_NAME = argv[1];
    const char         *PORT      = argv[2];
    const char         *FILE_NAME = argv[3];
    struct in_addr     sip_addr;
    struct hostent     *hp;
    struct stat        st;


    if (argc != 4) {
        printf("Usage : ftpc <remote-IP> <remote-port> <local-file-to-transfer>");
        exit(1);
    }

    /* initialize socket connection in unix domain */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error opening datagram socket");
        exit(1);
    }


    if (PORT == 0) {
        fprintf(stderr, "%s: unknown host\n", argv[1]);
        exit(2);
    }

    /* construct name of socket to send to */
    hp = gethostbyname(HOST_NAME);
    bcopy((void *) hp->h_addr, (void *) &sin_addr.sin_addr, hp->h_length);
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port   = htons(atoi(PORT));

    /* establish connection with server */
    if (connect(sock, (struct sockaddr *) &sin_addr, sizeof(struct sockaddr_in)) < 0) {
        close(sock);
        perror("error connecting stream socket");
        exit(1);
    }

    /* receive a message */
    if (recv(sock, buf_in, BUFFER_SIZE, 0) < 0) {
        perror("Error sending message to client");
        exit(1);
    }
    printf("%s\n", buf_in);

    /* send file size */
    fp = fopen(FILE_NAME, "rb");
    if (fp == NULL) {
        perror("Error:");
        exit(1);
    }
    if (stat(FILE_NAME, &st) >= 0) {
        file_size = htonl(st.st_size);
    }
    bzero(buf_out, BUFFER_SIZE);
    memcpy(buf_out, &file_size, FILE_SIZE_LENGTH);
    if (send(sock, buf_out, FILE_SIZE_LENGTH, 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }

    bzero(buf_out, FILE_SIZE_LENGTH);

    /* send file name */
    strncpy(buf_out, FILE_NAME, strlen(FILE_NAME));
    if (send(sock, buf_out, FILE_NAME_LENGTH, 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }
    printf("File name: %s, size: %ld\n", buf_out, ntohl(file_size));
    bzero(buf_out, FILE_NAME_LENGTH);

    /* send file */
    int current_len = 0;
    while ((current_len = fread(buf_out, 1, BUFFER_SIZE, fp)) > 0) {
        send(sock, buf_out, current_len, 0);
        bzero(buf_out, BUFFER_SIZE);
    }
    /*if(current_len < 0) {
        perror("Error sending file to server");
        exit(1);
    }*/
    fclose(fp);

    /* print confirmation msg */
    memcpy(&sip_addr, &sin_addr.sin_addr, IP_ADDR_LENGTH);
    printf("Sent %s to server %s: %s\n", FILE_NAME, inet_ntoa(sip_addr), PORT);

    /* close all connections and remove socket file */
    close(sock);
    return 0;
}