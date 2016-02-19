/* client.c using TCPD */

#include <libgen.h>
#include "tcp_daemon/headers/tcpd.h"

/* client program called with host name where server is run */
int main(int argc, char **argv) {
    int                sock;                                /* initial socket descriptor */
    struct sockaddr_in sin_addr;                            /* structure for socket name setup */
    FILE               *fp;                                 /* file sent to server */
    unsigned long      file_size  = 0;                      /* initialize file size */
    const char         *HOST_NAME = argv[1];                /* host name */
    const char         *PORT      = argv[2];                /* port number */
    const char         *FILE_NAME = argv[3];                /* file name */
    const char         *BASE_NAME = basename(argv[3]);      /* base file name */
    struct in_addr     sip_addr;                            /* structure for server ip address */
    struct hostent     *hp;                                 /* structure host information */
    struct stat        st;                                  /* structure file information */
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
    bzero(message.contents, CONTENT_BUFF_SIZE);
    memcpy(message.contents, &file_size, FILE_SIZE_LENGTH);

    if (SEND(sock, (char *) &message, FILE_SIZE_LENGTH + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }
    puts(message.contents);
    bzero(message.contents, FILE_SIZE_LENGTH);

    /* send file name */
    strncpy(message.contents, BASE_NAME, strlen(BASE_NAME));

    if (SEND(sock, (char *) &message, FILE_NAME_LENGTH + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }
    printf("File name: %s, size: %d\n", message.contents, ntohl(file_size));
    bzero(message.contents, FILE_NAME_LENGTH);

    /* send file */
    int current_len = 0;
    while ((current_len = fread(message.contents, 1, CONTENT_BUFF_SIZE, fp)) > 0) {
        SEND(sock, (char *) &message, current_len + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0);
        //usleep(10000);
        sleep(1);
    }

    fclose(fp);

    /* print confirmation msg */
    memcpy(&sip_addr, &sin_addr.sin_addr, IP_ADDR_LENGTH);
    printf("Sent %s to server %s: %s\n", FILE_NAME, inet_ntoa(sip_addr), PORT);

    /* close all connections and remove socket file */
    close(sock);
    return 0;
}
