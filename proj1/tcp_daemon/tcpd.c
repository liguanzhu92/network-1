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
    TcpdMessage        message, ctrl_msg, ack_msg;                                //Packet format accepted by troll
    NetMessage         troll_message;
    int client_sock, troll_sock, ctrl_sock, ack_sock, timer_send_sock, timer_recv_sock;   //Initial socket descriptors
    //Structures for server and tcpd socket name setup
    struct sockaddr_in client_addr, troll_addr, ctrl_addr, ack_addr, timer_send_addr, timer_recv_addr, server_addr;
    int new_buffer = SOCK_BUF_SIZE;

    /* initialize everything */
    __init_client_sock_c(client_sock, client_addr);
    __init_ctrl_sock_c(ctrl_sock, ctrl_addr);
    __init_ack_sock_c(ack_sock, ack_addr, new_buffer);
    __init_timer_send_sock_c(timer_send_sock, timer_send_addr);
    __init_timer_recv_sock_c(timer_recv_sock, timer_recv_addr, new_buffer);
    __init_troll_sock_c(troll_sock, troll_addr);

    //To hold the length of client_addr
    int len = sizeof(client_addr);

    //Counter to count number of datagrams forwarded
    int count = 0;

    bzero(troll_message.msg_contents, sizeof(message));

    //Always keep on listening and sending
    while (1) {

        //Receiving from ftpc
        int rec = (int)recvfrom(client_sock, &message, sizeof(message), 0, (struct sockaddr *) &client_addr, &len);
        server_addr = message.header;
        server_addr.sin_port       = htons(TCPD_PORT_S);
        troll_message.msg_header = server_addr;

        if (rec < 0) {
            perror("Error receiving datagram");
            exit(1);
        }

        printf("Received data from client, sending to troll --> %d\n", count);

        bzero(&message.tcp_header.check, sizeof(u_int16_t));
        message.tcp_header.check = htons(cal_crc((unsigned char *) &message, (unsigned short) rec));
        printf("check_sum: %hu\n", ntohs(message.tcp_header.check));
        bcopy((char *) &message, &troll_message.msg_contents, rec);
        //puts(message.contents);
        //Sending to troll
        int s = (int)sendto(troll_sock, &troll_message, rec + TCPD_HEADER_LENGTH, 0, (struct sockaddr *) &troll_addr, sizeof(troll_addr));

        if (s < 0) {
            perror("Error sending datagram");
            exit(1);
        }

        //Incrementing counter
        count++;

    }
}

void __init_client_sock_c(int client_sock, struct sockaddr_in client_addr) {
    if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error openting datagram socket");
        exit(1);
    }
    printf("client socket initialized \n");
    //Constructing socket name for receiving
    client_addr.sin_family      = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;            //Listen to any IP address
    client_addr.sin_port        = htons(TCPD_PORT_C);
    //Binding socket name to socket
    if (bind(client_sock, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding stream socket");
        exit(1);
    }
    printf("client socket name binded, waiting for client ...\n");
}

void __init_ctrl_sock_c(int ctrl_sock, struct sockaddr_in ctrl_addr) {
    if((ctrl_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening control socket.");
        exit(1);
    }
    printf("control socket initialized \n");
    ctrl_addr.sin_family = AF_INET;
    ctrl_addr.sin_port = htons(CTRL_PORT);
    ctrl_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
}

void __init_ack_sock_c(int ack_sock, struct sockaddr_in ack_addr, int new_buff) {
    if((ack_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from ftpc");
        exit(1);
    }
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT_C);
    ack_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(ack_sock, (struct sockaddr *)&ack_addr, sizeof(ack_addr)) < 0) {
        perror("ACK socket Bind failed");
        exit(2);
    }

    setsockopt(ack_sock, SOL_SOCKET, SO_RCVBUF, &new_buff, sizeof(&new_buff));
}

void __init_timer_send_sock_c(int timer_send_sock, struct sockaddr_in timer_send_addr) {
    if((timer_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from timer send");
        exit(1);
    }
    timer_send_addr.sin_family = AF_INET;
    timer_send_addr.sin_port = htons(TIMER_PORT_SERVER);
    timer_send_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);

    if((timer_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from timer recv");
        exit(1);
    }
}

void __init_timer_recv_sock_c(int timer_recv_sock, struct sockaddr_in timer_recv_addr, int new_buff) {
    if((timer_recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from timer send");
        exit(1);
    }
    timer_recv_addr.sin_family = AF_INET;
    timer_recv_addr.sin_port = htons(TIMER_PORT_SERVER);
    timer_recv_addr.sin_addr.s_addr = INADDR_ANY;

    if((timer_recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from timer recv");
        exit(1);
    }
    setsockopt(timer_recv_sock, SOL_SOCKET, SO_RCVBUF, &new_buff, sizeof(&new_buff));
}

void __init_troll_sock_c(int troll_sock, struct sockaddr_in troll_addr) {
    if((troll_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening troll socket");
        exit(1);
    }

    //Constructing socket name of the troll to send to
    troll_addr.sin_family      = AF_INET;
    troll_addr.sin_port        = htons(TROLL_PORT);
    troll_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
}