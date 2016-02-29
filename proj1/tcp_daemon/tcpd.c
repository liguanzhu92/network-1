#include "headers/tcpd.h"
#include "headers/troll.h"
#include "headers/check_sum.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage(server): %s -s, or\n", argv[0]);
        printf("Usage(client): %s -c\n", argv[0]);
        exit(1);
    }
    if (strcmp(argv[1], "-s") == 0) {
        tcpd_server();
    } else if (strcmp(argv[1], "-c") == 0) {
        tcpd_client();
    }
    return 0;
}

void tcpd_server() {
    TcpdMessage        tcpd_msg;
    NetMessage         troll_msg;
    int                sock, srv_sock;
    struct sockaddr_in my_addr, server_addr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error openting datagram socket");
        exit(1);
    }
    printf("Server socket initialized \n");

    //Copying socket to send to ftps
    srv_sock = sock;

    //Constructing socket name for receiving
    my_addr.sin_family      = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;            //Listen to any IP address
    my_addr.sin_port        = htons(TCPD_PORT_S);

    //Binding socket name to socket
    if (bind(sock, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding stream socket");
        exit(1);
    }
    printf("Socket binded, wait for troll data...\n");

    //To hold the length of my_addr
    int len = sizeof(my_addr);

    //Counter to count number of datagrams forwarded
    int count = 0;

    //Always keep on listening and sending
    while (1) {
        //Receiving from troll
        int rec = (int)recvfrom(sock, &troll_msg, sizeof(troll_msg), 0, (struct sockaddr *) &my_addr, &len);

        if (rec < 0) {
            perror("Error receiving datagram");
            close(sock);
            close(srv_sock);
            exit(1);
        }

        bcopy(&troll_msg.msg_contents, &tcpd_msg, rec - TCPD_HEADER_LENGTH);

        server_addr = tcpd_msg.header;
        server_addr.sin_family      = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
        ////Sending to ftps
        int s = (int)sendto(srv_sock,
                       tcpd_msg.contents,
                       rec - TCPD_HEADER_LENGTH * 2 - TCP_HEADER_LENGTH ,
                       0,
                       (struct sockaddr *) &server_addr,
                       sizeof(server_addr));

        //puts(tcpd_msg.contents);
        if (s < 0) {
            perror("Error sending datagram");
            close(sock);
            close(srv_sock);
            exit(1);
        }

        printf("Received from troll ,sending to server --> %d\n", count);
        // compare the content
        unsigned short tcpd_check_sum = ntohs(tcpd_msg.tcp_header.check);
        bzero(&tcpd_msg.tcp_header.check, sizeof(u_int16_t));
        unsigned short local_check_sum = cal_crc((unsigned char *)&tcpd_msg, (unsigned short)(rec - TCPD_HEADER_LENGTH));
        printf("tcpd_check_sum:%hu\n", tcpd_check_sum);
        printf("local_check_sum:%hu\n", local_check_sum);
        if (tcpd_check_sum != local_check_sum){
            printf("%sgarbling detected!%s\n", "\x1B[33m", "\x1B[0m");
        }

        //Incrementing counter
        count++;

    }
}

void tcpd_client() {
    TcpdMessage        message;                                //Packet format accepted by troll
    NetMessage         troll_message;
    int                sock, troll_sock;                               //Initial socket descriptors
    struct sockaddr_in troll, my_addr, ftps_addr;                    //Structures for server and tcpd socket name setup

    //Initialize socket for UDP in linux
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error openting datagram socket");
        exit(1);
    }
    printf("Socket initialized \n");

    //Copying socket to send to troll
    troll_sock = sock;

    //Constructing socket name for receiving
    my_addr.sin_family      = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;            //Listen to any IP address
    my_addr.sin_port        = htons(TCPD_PORT);

    //Constructing socket name of the troll to send to
    troll.sin_family      = AF_INET;
    troll.sin_port        = htons(TROLL_PORT);
    troll.sin_addr.s_addr = inet_addr(LOCAL_HOST);

    //Binding socket name to socket
    if (bind(sock, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding stream socket");
        close(sock);
        close(troll_sock);
        exit(1);
    }
    printf("Socket name binded, waiting for client ...\n");

    //To hold the length of my_addr
    int len = sizeof(my_addr);

    //Counter to count number of datagrams forwarded
    int count = 0;

    bzero(troll_message.msg_contents, sizeof(message));

    //Always keep on listening and sending
    while (1) {

        //Receiving from ftpc
        int rec = (int)recvfrom(sock, &message, sizeof(message), 0, (struct sockaddr *) &my_addr, &len);
        ftps_addr = message.header;
        ftps_addr.sin_port       = htons(TCPD_PORT_S);
        troll_message.msg_header = ftps_addr;

        if (rec < 0) {
            perror("Error receiving datagram");
            close(sock);
            close(troll_sock);
            exit(1);
        }

        printf("Received data from client, sending to troll --> %d\n", count);

        bzero(&message.tcp_header.check, sizeof(u_int16_t));
        message.tcp_header.check = htons(cal_crc((unsigned char *) &message, (unsigned short) rec));
        printf("check_sum: %hu\n", ntohs(message.tcp_header.check));
        bcopy((char *) &message, &troll_message.msg_contents, rec);
        //puts(message.contents);
        //Sending to troll
        int s = (int)sendto(troll_sock, &troll_message, rec + TCPD_HEADER_LENGTH, 0, (struct sockaddr *) &troll, sizeof(troll));

        if (s < 0) {
            perror("Error sending datagram");
            close(sock);
            close(troll_sock);
            exit(1);
        }

        //Incrementing counter
        count++;

    }
}
