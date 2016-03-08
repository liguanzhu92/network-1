#include <sys/time.h>
#include "headers/tcpd.h"
#include "headers/troll.h"
#include "headers/check_sum.h"
#include "headers/delta_list.h"

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

        server_addr = tcpd_msg.tcpd_header;
        server_addr.sin_family      = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
        ////Sending to ftps
        int s = (int)sendto(srv_sock,
                       tcpd_msg.contents,
                       rec - TCPD_HEADER_LENGTH * 2 - TCP_HEADER_LENGTH,
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
    fd_set fd_read;
    int window[WINDOW_SIZE];
    TcpdMessage tcpd_buf[TCPD_BUF_SIZE];
    int window_index = 0, head = 0, index = 0;
    struct timeval start_time, end_time;
    time_message timer_send_message, timer_recv_message;
    float time_remain = 0;
    int resend_pack = -1;

    /* initialize everything */
    __init_client_sock_c(client_sock, client_addr);
    __init_ctrl_sock_c(ctrl_sock, ctrl_addr);
    __init_ack_sock_c(ack_sock, ack_addr, new_buffer);
    __init_timer_send_sock_c(timer_send_sock, timer_send_addr);
    __init_timer_recv_sock_c(timer_recv_sock, timer_recv_addr, new_buffer);
    __init_troll_sock_c(troll_sock, troll_addr);

    //To hold the length of client_addr
    int len = sizeof(struct sockaddr_in);

    //Counter to count number of datagrams forwarded
    int count = 0;

    bzero(troll_message.msg_contents, sizeof(message));

    FD_ZERO(&fd_read);
    FD_SET(client_sock, &fd_read);
    FD_SET(ack_sock, &fd_read);
    FD_SET(timer_recv_sock, &fd_read);

    /* initialize window and buffer */
    for(int i = 0; i < WINDOW_SIZE; i++) {
        window[i] = -1;
    }
    for(int i = 0; i < TCPD_BUF_SIZE; i++) {
        tcpd_buf->tcp_header.seq = 0xFFFFFFFF;
    }

    //Always keep on listening and sending
    while (TRUE) {
        if(select(FD_SETSIZE, &fd_read, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(0);
        }

        /* send data to troll */
        if(FD_ISSET(client_sock, &fd_read)) {
            ssize_t rec = recvfrom(client_sock, (void *) &tcpd_buf[head], TCPD_MESSAGE_SIZE, 0, (struct sockaddr *) &client_addr, &len);
            printf("\nReceived seq_num %u from client\n", tcpd_buf[head].tcp_header.seq);

            window[window_index] = tcpd_buf[head].tcp_header.seq;

            /* modify the tcpd_header so it can be sent by troll to tcpd_s */
            server_addr = message.tcpd_header;
            server_addr.sin_port = htons(TCPD_PORT_S);
            tcpd_buf[head].tcpd_header = server_addr;

            /* calculate crc */
            bzero(&tcpd_buf[head].tcp_header.check, sizeof(u_int16_t));
            tcpd_buf[head].tcp_header.check = htons(cal_crc((unsigned char *) &tcpd_buf[head], (unsigned short) rec));
            printf("check_sum: %hu\n", ntohs(tcpd_buf[head].tcp_header.check));

            // TODO may not need this
            for(int i = 0; i < TCPD_BUF_SIZE; i++) {
                if(tcpd_buf[i].tcp_header.seq == window[window_index]) {
                    index = i;
                    break;
                }
            }
            tcpd_buf[index].tcp_header.window = WINDOW_SIZE - window_index;

            /* send to troll */
            if(sendto(troll_sock, (void *) &tcpd_buf[index], rec + TCPD_HEADER_LENGTH, 0, (struct sockaddr *) &troll_addr, len) < 0) {
                perror("send from tcpd_c to troll");
                exit(0);
            }
            printf("\nsending seq %u from tcpd_c to troll", tcpd_buf[index].tcp_header.seq);

            /* send to timer */
            gettimeofday(&start_time, NULL);
            timer_send_message.action = START;
            timer_send_message.seq_num = tcpd_buf[index].tcp_header.seq;
            timer_send_message.time = cal_RTO(time_remain, tcpd_buf[index].tcp_header.seq) * 10;
            printf("\nsending seq %u to timer\n", tcpd_buf[index].tcp_header.seq);
            sendto(timer_send_sock, &timer_send_message, sizeof(time_message), 0, (struct sockaddr *) &timer_send_addr, len);

            /* move window index and wrap buffer*/
            window_index++;
            head = (head + 1) % TCPD_BUF_SIZE;

            /* send to control socket */
            if(window_index >= WINDOW_SIZE) {
                printf("\nWINDOW FULL, SLEEP\n");
            } else {
                printf("\nWINDOW NOT FULL, KEEP SENDING\n");
            }
            ctrl_msg.tcp_header.window = window_index;
            sendto(ctrl_sock, (void *)&ctrl_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&ctrl_addr, len);
        }

        /* received or retransmission */
        if(FD_ISSET(ack_sock, &fd_read)) {
            printf("\ngetting ack\n");
            gettimeofday(&end_time, NULL);
            time_remain = cal_RTT(&start_time, &end_time);
            printf("\n RTT IS %f\n", time_remain);

            unsigned short ack_crc;
            unsigned short ack_cal_crc;
            ssize_t rec = recvfrom(ack_sock, (void*)&ack_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&ack_addr, &len);

            ack_crc = ntohs(ack_msg.tcp_header.check);
            bzero(&ack_msg.tcp_header.check, sizeof(u_int16_t));
            ack_cal_crc = cal_crc((char *) &ack_msg, rec);
            printf("\ncal checksum: %hu, received checksum: %hu\n", ack_crc, ack_cal_crc);
            printf("\nACK SEQ: %d\n", ack_msg.tcp_header.seq);

            if(ack_crc == ack_cal_crc) {
                if(ack_msg.tcp_header.ack == 1) {

                    //Delete this seq from Timer
                    timer_send_message.seq_num = ack_msg.tcp_header.ack_seq;
                    timer_send_message.action = CANCEL;
                    timer_send_message.time = 0;
                    printf("\ncancel timer\n");
                    sendto(timer_send_sock, &timer_send_message, sizeof(time_message), 0, (struct sockaddr *) &timer_send_addr, len);

                    for(int i = 0; i < WINDOW_SIZE; i++) {
                        if(window[i] == ack_msg.tcp_header.ack_seq) {
                            printf("I've cleaned window: window[%d]\n, WINDOWS[i]: %d, SEQ: %d", i, window[i], ack_msg.tcp_header.ack_seq);
                            window[i] = 0xFFFFFFFF;//RECV ACK
                        }
                    }

                    if(is_window_empty(window)) {
                        printf("\nWINDOW EMPTY\n");
                        ctrl_msg.tcp_header.window = 0;//WINDOW EMPTY, KEEP SENDING
                        window_index = 0;//Make pointer back to begin
                        sendto(ctrl_sock, (void *)&ctrl_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&ctrl_addr, len);
                    }
                }
                if(ack_msg.tcp_header.fin == 1) {
                    printf("\nfinish!\n");
                    recvfrom(ack_sock, (void *)&ack_msg, sizeof(ack_msg), 0, (struct sockaddr *) &ack_msg, &len);//WAITING FOR FIN ACK
                    if(strcmp(ack_msg.contents, "FIN")) {
                        ctrl_msg.tcp_header.window = WINDOW_SIZE;
                        sendto(ctrl_sock, (void*)&ctrl_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&ctrl_addr, len);

                        timer_send_message.seq_num = ack_msg.tcp_header.ack_seq;
                        timer_send_message.action = CANCEL;
                        timer_send_message.time = 0;
                        printf("\ncancel node seq: %d\n", timer_send_message.seq_num);
                        sendto(timer_send_sock, &timer_send_message, sizeof(timer_send_message), 0, (struct sockaddr *) &timer_send_addr, len);
                        close(troll_sock);
                        close(client_sock);
                        exit(0);
                    }
                }
            }
        }

        /* if something expired */
        if(FD_ISSET(timer_recv_sock, &fd_read)) {
            printf("\nIn timer recv\n");

            if(recvfrom(timer_recv_sock, &timer_recv_message, sizeof(timer_recv_message), 0, (struct sockaddr *)&timer_recv_addr, &len) > 0) {
                printf("\n PACKET SEQ NUM: %d HAS EXPIRED\n", timer_recv_message.seq_num);

            }

            for(int i = 0; i < WINDOW_SIZE; i++) {
                if(window[i] == timer_recv_message.seq_num) {
                    for(int j = 0; j < TCPD_BUF_SIZE; j++) {
                        if((tcpd_buf[j].tcp_header.seq == timer_recv_message.seq_num) && (tcpd_buf[j].tcp_header.seq != 0xFFFFFFFF)) {
                            printf("\nRESEND TO BUFFER\n");
                            resend_pack = j;
                        }
                    }//END
                }//END if window
            }

            if(resend_pack != -1) {
                /* send to troll */
                sendto(troll_sock, (void *)&tcpd_buf[resend_pack], TCPD_BUF_SIZE, 0, (struct sockaddr *)&troll_addr, sizeof(troll_addr));

                /* send to timer */
                gettimeofday(&start_time, NULL);
                timer_send_message.time = cal_RTO(time_remain, tcpd_buf[resend_pack].tcp_header.seq)*10;
                timer_send_message.seq_num = tcpd_buf[resend_pack].tcp_header.seq;
                timer_send_message.action = START;
                sendto(timer_send_sock, &timer_send_message, sizeof(timer_send_message), 0, (struct sockaddr*)&timer_send_addr, len);
                resend_pack = -1;
            }
        }

        /* reset */
        FD_ZERO(&fd_read);
        FD_SET(client_sock, &fd_read);
        FD_SET(ack_sock, &fd_read);
        FD_SET(timer_recv_sock, &fd_read);
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

int is_window_empty(int window[]) {
    for(int i = 0; i < WINDOW_SIZE; i++) {
        if(window[i] != 0xFFFFFFFF) {
            return FALSE;
        }
    }
    return FALSE;
}