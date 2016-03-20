/* client.c using TCPD */

#include <libgen.h>
#include "tcp_daemon/headers/tcpd.h"

/* client program called with host name where server is run */
int main(int argc, char **argv) {
    int sock;                                /* initial socket descriptor */
    struct sockaddr_in sin_addr;                            /* structure for socket name setup */
    FILE *fp;                                 /* file sent to server */
    unsigned long file_size = 0;                      /* initialize file size */
    const char *HOST_NAME = argv[1];                /* host name */
    const char *PORT = argv[2];                /* port number */
    const char *FILE_NAME = argv[3];                /* file name */
    const char *BASE_NAME = basename(argv[3]);      /* base file name */
    struct in_addr sip_addr;                            /* structure for server ip address */
    struct hostent *hp;                                 /* structure host information */
    struct stat st;                                  /* structure file information */
    TcpdMessage message;
    fd_set fd_read;
    int ctrl_sock;
    struct sockaddr_in ctrl_addr;
    TcpdMessage ctrl_recv_message;
    int EOF_FILE = 0;
    int read_len;


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
    if((ctrl_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
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
    sin_addr.sin_port = htons(atoi(PORT));

    /* set up control socket */
    ctrl_addr.sin_family = AF_INET;
    ctrl_addr.sin_port = htons(CTRL_PORT);
    ctrl_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(ctrl_sock, (struct sockaddr *) &ctrl_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("error binding stream socket");
        exit(1);
    }
    int new_buf_size = SOCK_BUF_SIZE;
    setsockopt(ctrl_sock, SOL_SOCKET, SO_RCVBUF, &new_buf_size, sizeof(&new_buf_size));

    message.tcpd_header = sin_addr;

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
    message.tcp_header.seq = 0;

    if (SEND(sock, (char *) &message, FILE_SIZE_LENGTH + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }
    printf("Transferring file size ---> SEQ: %d\n", message.tcp_header.seq);
    puts(message.contents);
    bzero(message.contents, FILE_SIZE_LENGTH);

    /* send file name */
    strncpy(message.contents, BASE_NAME, strlen(BASE_NAME));
    message.tcp_header.seq++;

    if (SEND(sock, (char *) &message, FILE_NAME_LENGTH + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0) < 0) {
        perror("Error sending message from client");
        exit(1);
    }
    printf("Transferring file name ---> SEQ: %d\n", message.tcp_header.seq);
    printf("File name: %s, size: %d\n", message.contents, ntohl(file_size));
    bzero(message.contents, FILE_NAME_LENGTH);

    FD_ZERO(&fd_read);
    FD_SET(ctrl_sock, &fd_read);
    /* send file */
    while (1) {
        printf("Transferring file contents ---> ");
        if (select(FD_SETSIZE, &fd_read, NULL, NULL, NULL) < 0) {
            perror("select in client");
            exit(1);
        }
        if (FD_ISSET(ctrl_sock, &fd_read)) {
            RECV_CTRL(ctrl_sock, &ctrl_recv_message, TCPD_MESSAGE_SIZE, 0);
            if (ctrl_recv_message.tcp_header.window >= WINDOW_SIZE) {
                printf("window is full, stop sending\n");
                usleep(100000);
            } else {
                bzero(message.contents, CONTENT_BUFF_SIZE);
                if ((read_len = fread(message.contents, 1, CONTENT_BUFF_SIZE, fp)) < CONTENT_BUFF_SIZE) {
                    message.tcp_header.seq++;
                    SEND(sock, (char *) &message, read_len + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0);
                    //usleep(10000);
                    sleep(1);
                    EOF_FILE = 1;
                    printf("LAST SEQ: %d\n", message.tcp_header.seq);
                } else {
                    message.tcp_header.seq++;
                    message.tcp_header.fin = 0;
                    //usleep(10000);
                    sleep(1);
                    SEND(sock, (char *) &message, read_len + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0);
                    printf("SEQ: %d\n", message.tcp_header.seq);
                }
                if (EOF_FILE == 1) {
                    bzero(message.contents, CONTENT_BUFF_SIZE);
                    message.tcp_header.fin = 1;
                    message.tcp_header.seq++;
                    SEND(sock, (char *) &message, read_len + TCPD_HEADER_LENGTH + TCP_HEADER_LENGTH, 0);
                    close(sock);
                    fclose(fp);
                    exit(0);
                }

            }
        }
        FD_ZERO(&fd_read);
        FD_SET(ctrl_sock, &fd_read);
    }
    return 0;
}
