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
    TcpdMessage        tcpd_recv[TCPD_BUF_SIZE], tcpd_send, ack_msg;
    NetMessage         troll_message;
    int sever_sock, troll_s_sock, troll_c_sock, ack_sock;   //Initial socket descriptors
    //Structures for server and tcpd socket name setup
    struct sockaddr_in troll_s_addr, troll_c_addr, ack_addr, server_addr;
    int new_buffer = SOCK_BUF_SIZE;
    int ack_buffer[TCPD_BUF_SIZE];
    fd_set fd_read;
    int window[WINDOW_SIZE];
    int window_index = 0, head = 0, index = 0;
    //FLAGS
    int crc_match = FALSE;
    int ack_buffer_flag = FALSE;
    int ack_exist = FALSE;
    int checksum = 0;
    int nr_failed_acks = 0;
    int lastsent = -1; //check sequence whether received

    /* initialize window and ack buffer */
    for(int i = 0; i < WINDOW_SIZE; i++) {
        window[i] = -1;
    }
    for(int i = 0; i < TCPD_BUF_SIZE; i++) {
        ack_buffer[i] = -1;
    }

    /*create from ftps socket*/
    if((sever_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for send to ftps");
        exit(1);
    }

    /*create troll socket*/
    if((troll_c_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from troll m2");
        exit(1);
    }
    printf("Server socket initialized \n");

    /* create troll_addr with parameters and bind troll_addr to socket */
    /*bind to the troll - M2*/
    troll_c_addr.sin_family = AF_INET;
    troll_c_addr.sin_port = htons(TCPD_PORT_S);
    troll_c_addr.sin_addr.s_addr = INADDR_ANY;

    troll_s_addr.sin_family = AF_INET;
    troll_s_addr.sin_port = htons(TROLL_PORT_SERVER);
    troll_s_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);

    //Binding socket name to socket
    if(bind(troll_c_sock, (struct sockaddr *)&troll_c_addr, sizeof(troll_c_addr)) < 0) {
        perror("Recv(receive from troll socket Bind failed");
        exit(2);
    }

    setsockopt(troll_c_sock, SOL_SOCKET, SO_RCVBUF, &new_buffer , sizeof(&new_buffer ));

    if((ack_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for send ack to tcpd_m2");
        exit(1);
    }
    printf("Socket binded, wait for troll data...\n");


    // Construct troll header
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT_C);
    ack_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCPD_PORT_S);
    server_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);

    //To hold the length of sever_addr
    int len = sizeof(struct sockaddr_in);

    //Counter to count number of datagrams forwarded
    int count = 0;

    FD_ZERO(&fd_read);
    FD_SET(troll_c_sock, &fd_read);

    //Always keep on listening and sending
    while (TRUE) {
        if(select(FD_SETSIZE, &fd_read, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(0);
        }
        //Receiving from troll- client
        if(FD_ISSET(troll_c_sock, &fd_read)){
            int rec = 0;
            if((rec = recvfrom(troll_c_sock, &troll_message, sizeof(NetMessage), 0, (struct sockaddr *)&troll_c_addr, &len)) < 0){
                perror("RECV from troll error");
                exit(0);
            }
            rec -= TCPD_HEADER_LENGTH;
            memcpy((void *)&tcpd_recv[head], &troll_message.msg_contents, rec);
            // check sequence received
            /*out of window bound*/

            int seq = tcpd_recv[head].tcp_header.seq;
            int lowest_seq;
            int highest_seq;
            int IsAccept;

            if (lastsent < 0) {
                IsAccept = 1;
            } else {
                lowest_seq = (lastsent / 20) * 20;
                highest_seq = lowest_seq + 20;
                if ((lastsent + 1) % 20 == 0) {
                    lowest_seq = highest_seq;
                    highest_seq = lowest_seq + 20;
                }

                if (seq < lowest_seq || seq >= highest_seq)
                    IsAccept = 0;
                else
                    IsAccept = 1;
            }

            printf("----------------------\n");
            printf("accept: %d, lowest: %d, highest: %d, seq: %d, lastsent: %d\n", IsAccept, lowest_seq, highest_seq, seq, lastsent);
            printf("----------------------\n");

            if (!IsAccept) {
                printf("OUT OF window BOUND\n");
                FD_ZERO(&fd_read);
                FD_SET(troll_c_sock,&fd_read);
                continue;
            }

            u_int16_t check_in_message = tcpd_recv[head].tcp_header.check;
            bzero(&tcpd_recv[head].tcp_header.check, sizeof(u_int16_t));
            checksum = cal_crc((char *) &tcpd_recv[head], (unsigned short) rec);// no idea about the length

            if(checksum == check_in_message) {
                crc_match = TRUE;
                ack_buffer_flag = FALSE;
                // check ack
                for(int i = 0; i< TCPD_BUF_SIZE; i++) {
                    if (ack_buffer[i] == tcpd_recv[head].tcp_header.seq) {
                        ack_buffer_flag = TRUE;
                        break;
                    }
                }
                    // not in buffer
                if(ack_buffer_flag != TRUE){
                    ack_exist = FALSE;
                    for(int i = 0; i < WINDOW_SIZE; i++) {
                        //CHECK WHETHER IN WINDOW OR NOT
                        if (window[i] == tcpd_recv[head].tcp_header.seq) {
                            ack_exist = TRUE;
                            break;
                        }
                    }
                    if(ack_exist){
                        printf("\nIN THE WINDOWS, %d WILL ARRIVE FTPS SOON\n",tcpd_recv[head].tcp_header.seq);
                    } else { //ack_exist == FALSE
                        window_index = tcpd_recv[head].tcp_header.seq % 20;
                        window[window_index] = tcpd_recv[head].tcp_header.seq ;
                        printf("\nWIN SRV [%d]: %d\n",window_index, tcpd_recv[head].tcp_header.seq);
                        if(head < TCPD_BUF_SIZE - 1) {
                            head++;
                        } else {
                            head = 0;//Make buffer zero
                        }
                    }
                } else {
                    //situation: ack_buffer_flag == FALSE
                    //construct ack_msg for re-sent;
                    bzero(&ack_msg.tcp_header.check,sizeof(u_int16_t));
                    ack_msg.tcp_header.ack = 1;
                    ack_msg.tcp_header.ack_seq = tcpd_recv[head].tcp_header.seq;
                    ack_msg.tcpd_header = ack_addr;
                    ack_msg.tcp_header.check = cal_crc((char *) &ack_msg, (unsigned short) TCPD_MESSAGE_SIZE);//no idea size
                    if(sendto(ack_sock, (void *)&ack_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&troll_s_addr, sizeof(troll_s_addr)) < 0) {
                        perror("send to sock_ack error");
                        exit(0);
                    }
                    printf("\nSEND ACK SEQ %d, TO TROLL M1\n", ack_msg.tcp_header.ack_seq);
                    }
            } else {
                crc_match = FALSE;//checksum wrong
                printf("\nCRC HAVE SOMETHING WRONG\n");
            }
            if (crc_match){
                // find the lowest_seq
                lowest_seq = 100000;
                int lowest_seq_window_index = -1;
                for(int i = 0; i < WINDOW_SIZE; i++) {
                    if((window[i] < lowest_seq) && (window[i] != -1)) {
                        lowest_seq = window[i];
                        lowest_seq_window_index = i;
                    }
                }
                printf("\nLOWEST_SEQ %d\n", lowest_seq);
                printf("\nLASTSENT: %d\n", lastsent);
                //if lowest in win is to be sent
                if(lowest_seq == (lastsent + 1)){
                    int buffer_index = 0;
                    int pointer = 0;
                    for(int i = 0; i < 64; i++) {
                        if (tcpd_recv[i].tcp_header.seq == lowest_seq) {
                            buffer_index = i;
                            break;
                        }
                    }
                    if(sendto(sever_sock, (void *)&tcpd_recv[buffer_index], TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                        perror("Send to sock_ftps error");
                        exit(0);
                    }
                    lastsent = tcpd_recv[buffer_index].tcp_header.seq;
                    printf("[%d]finish bit: %d\n",
                               tcpd_recv[buffer_index].tcp_header.seq,
                               tcpd_recv[buffer_index].tcp_header.fin);
                    if(!tcpd_recv[buffer_index].tcp_header.fin){
                        bzero(&ack_msg.tcp_header.check, sizeof(u_int16_t));
                        ack_msg.tcp_header.ack = 1;
                        ack_msg.tcp_header.ack_seq = tcpd_recv[buffer_index].tcp_header.seq;
                        ack_buffer[pointer] = tcpd_recv[buffer_index].tcp_header.seq;
                        printf("\nPTR Value %d; lastsent = %d\n", pointer, lastsent);
                        pointer = (pointer + 1)%64;
                        ack_msg.tcp_header.check = cal_crc((char *) &ack_msg, (unsigned short)TCPD_MESSAGE_SIZE);// no idea
                        ack_msg.tcpd_header = ack_addr;
                        if(ack_msg.tcp_header.ack_seq % 5 == 0) {
                                sendto(ack_sock, (void *)&ack_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&troll_s_addr, sizeof(troll_s_addr));
                        } else {
                            if (nr_failed_acks > 5) {
                                sendto(ack_sock, (void *)&ack_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&troll_s_addr, sizeof(troll_s_addr));
                            }
                            nr_failed_acks++;
                        }
                        printf("\nACK SEQ SENT:%d\n", tcpd_recv[buffer_index].tcp_header.seq);
                        window[lowest_seq_window_index] = -1;
                    } else {
                        printf("\nRECEIVE FIN!!! seq: %d\n", tcpd_recv[buffer_index].tcp_header.seq);
                        sendto(sever_sock, (void*)&tcpd_recv[buffer_index], TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                        bzero(&ack_msg.tcp_header.check, sizeof(u_int16_t));
                        ack_msg.tcpd_header = ack_addr;
                        //ack_msg.tcp_header.fin = 1;
                        ack_msg.tcp_header.ack = 0;
                        ack_msg.tcp_header.ack_seq = tcpd_recv[buffer_index].tcp_header.seq;
                        ack_msg.tcp_header.check = cal_crc((char *) &ack_msg, (unsigned short) TCPD_MESSAGE_SIZE);
                        window[lowest_seq_window_index] = -1;
                        strcpy(ack_msg.contents, "FIN");
                        sendto(ack_sock, (void *)&ack_msg, TCPD_MESSAGE_SIZE, 0, (struct sockaddr *)&troll_s_addr, sizeof(troll_s_addr));
                        printf("\nFINISH TRANSFER FILE\n");
                        close(ack_sock);
                        close(troll_s_sock);
                        close(sever_sock);
                        exit(0);
                    }
                } else { //fin != 1
                    printf("\nSLEEP FOR WAITING\n");
                    usleep(100000);
                }
            } else { //crc_match == FALSE
                printf("\nCRC WRONG, RETRANSMIT\n");
            }
        }
        FD_ZERO(&fd_read);
        FD_SET(troll_c_sock, &fd_read);
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
    int length_buf[TCPD_BUF_SIZE];
    int window_index = 0, head = 0, index = 0;
    struct timeval start_time, end_time;
    time_message timer_send_message, timer_recv_message;
    float time_remain = 0;
    int resend_pack = -1;

    /* initialize everything */
    __init_client_sock_c(&client_sock, &client_addr);
    __init_ctrl_sock_c(&ctrl_sock, &ctrl_addr);
    __init_ack_sock_c(&ack_sock, &ack_addr, new_buffer);
    __init_timer_send_sock_c(&timer_send_sock, &timer_send_addr);
    __init_timer_recv_sock_c(&timer_recv_sock, &timer_recv_addr, new_buffer);
    __init_troll_sock_c(&troll_sock, &troll_addr);

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
        tcpd_buf[i].tcp_header.seq = 0xFFFFFFFF;
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
            length_buf[head] = (int)rec;
            printf("\nReceived seq_num %u from client\n", tcpd_buf[head].tcp_header.seq);

            window[window_index] = tcpd_buf[head].tcp_header.seq;

            /* modify the tcpd_header so it can be sent by troll to tcpd_s */
            server_addr = message.tcpd_header;
            server_addr.sin_port = htons(TCPD_PORT_S);
            server_addr.sin_family = AF_INET;
            tcpd_buf[head].tcpd_header = server_addr;

            // TODO may not need this
            for(int i = 0; i < TCPD_BUF_SIZE; i++) {
                if(tcpd_buf[i].tcp_header.seq == window[window_index]) {
                    index = i;
                    break;
                }
            }
            tcpd_buf[index].tcp_header.window = WINDOW_SIZE - window_index;

            /* calculate crc */
            bzero(&tcpd_buf[head].tcp_header.check, sizeof(u_int16_t));
            tcpd_buf[head].tcp_header.check = cal_crc((char *) &tcpd_buf[head], (unsigned short) rec);
            printf("check_sum: %hu\n", tcpd_buf[head].tcp_header.check);

            /* send to troll */
            troll_message.msg_header = tcpd_buf[index].tcpd_header;
            troll_message.msg_header.sin_family = htons(AF_INET);
            troll_message.msg_header.sin_port = htons(TCPD_PORT_S);
            memcpy((void *) &troll_message.msg_contents, (void *) &tcpd_buf[index], length_buf[index]);
            if(sendto(troll_sock, &troll_message, length_buf[index] + TCPD_HEADER_LENGTH, 0, (struct sockaddr *) &troll_addr, len) < 0) {
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

            ack_crc = ack_msg.tcp_header.check;
            bzero(&ack_msg.tcp_header.check, sizeof(u_int16_t));
            ack_cal_crc = cal_crc((char *) &ack_msg, (unsigned short)rec);
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
            //printf("\nIn timer recv\n");

            if(recvfrom(timer_recv_sock, &timer_recv_message, sizeof(timer_recv_message), 0, (struct sockaddr *)&timer_recv_addr, &len) > 0) {
                printf("\nPACKET SEQ NUM: %u HAS EXPIRED\n", timer_recv_message.seq_num);

            }

            for(int i = 0; i < WINDOW_SIZE; i++) {
                if(window[i] == timer_recv_message.seq_num) {
                    for(int j = 0; j < TCPD_BUF_SIZE; j++) {
                        if((tcpd_buf[j].tcp_header.seq == timer_recv_message.seq_num) && (tcpd_buf[j].tcp_header.seq != 0xFFFFFFFF)) {
                            resend_pack = j;
                        }
                    }//END
                }//END if window
            }

            if(resend_pack != -1) {
                /* send to troll */
                memcpy((void *) &troll_message.msg_contents, (void *) &tcpd_buf[resend_pack], length_buf[resend_pack]);
                troll_message.msg_header = tcpd_buf[resend_pack].tcpd_header;
                troll_message.msg_header.sin_family = htons(AF_INET);
                troll_message.msg_header.sin_port = htons(TCPD_PORT_S);
                sendto(troll_sock, &troll_message, length_buf[resend_pack] + TCPD_HEADER_LENGTH, 0, (struct sockaddr *)&troll_addr, sizeof(troll_addr));
                printf("sending seq %u from tcpd_c to troll\n", tcpd_buf[resend_pack].tcp_header.seq);

                /* send to timer */
                gettimeofday(&start_time, NULL);
                timer_send_message.time = cal_RTO(time_remain, tcpd_buf[resend_pack].tcp_header.seq)*10;
                timer_send_message.seq_num = tcpd_buf[resend_pack].tcp_header.seq;
                timer_send_message.action = START;
                sendto(timer_send_sock, &timer_send_message, sizeof(timer_send_message), 0, (struct sockaddr*)&timer_send_addr, len);
                printf("sending seq %u to timer\n", tcpd_buf[resend_pack].tcp_header.seq);
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

void __init_client_sock_c(int* client_sock, struct sockaddr_in* client_addr) {
    if ((*client_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error openting datagram socket");
        exit(1);
    }
    printf("client socket initialized \n");
    //Constructing socket name for receiving
    client_addr->sin_family      = AF_INET;
    client_addr->sin_addr.s_addr = INADDR_ANY;            //Listen to any IP address
    client_addr->sin_port        = htons(TCPD_PORT_C);
    //Binding socket name to socket
    if (bind(*client_sock, (struct sockaddr *) client_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding stream socket");
        exit(1);
    }
    printf("client socket name binded, waiting for client ...\n");
}

void __init_ctrl_sock_c(int* ctrl_sock, struct sockaddr_in* ctrl_addr) {
    if((*ctrl_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening control socket.");
        exit(1);
    }
    printf("control socket initialized \n");
    ctrl_addr->sin_family = AF_INET;
    ctrl_addr->sin_port = htons(CTRL_PORT);
    ctrl_addr->sin_addr.s_addr = inet_addr(LOCAL_HOST);
}

void __init_ack_sock_c(int* ack_sock, struct sockaddr_in* ack_addr, int new_buff) {
    if((*ack_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from ftpc");
        exit(1);
    }
    ack_addr->sin_family = AF_INET;
    ack_addr->sin_port = htons(ACK_PORT_C);
    ack_addr->sin_addr.s_addr = INADDR_ANY;
    printf("ack socket initialized \n");
    if(bind(*ack_sock, (struct sockaddr *) ack_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("ACK socket Bind failed");
        exit(2);
    }
    printf("ack socket binded \n");
    setsockopt(*ack_sock, SOL_SOCKET, SO_RCVBUF, &new_buff, sizeof(&new_buff));
}

void __init_timer_send_sock_c(int* timer_send_sock, struct sockaddr_in* timer_send_addr) {
    if((*timer_send_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from timer send");
        exit(1);
    }
    printf("timer_send socket initialized \n");
    timer_send_addr->sin_family = AF_INET;
    timer_send_addr->sin_port = htons(TIMER_PORT_SERVER);
    timer_send_addr->sin_addr.s_addr = inet_addr(LOCAL_HOST);
}

void __init_timer_recv_sock_c(int* timer_recv_sock, struct sockaddr_in* timer_recv_addr, int new_buff) {
    if((*timer_recv_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket for recv from timer send");
        exit(1);
    }
    printf("timer_recv socket initialized \n");
    timer_recv_addr->sin_family = AF_INET;
    timer_recv_addr->sin_port = htons(TIMER_PORT_CLIENT);
    timer_recv_addr->sin_addr.s_addr = INADDR_ANY;

    if(bind(*timer_recv_sock, (struct sockaddr *) timer_recv_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("timer_recv socket bind failed");
        exit(2);
    }
    printf("timer_recv socket binded \n");
    setsockopt(*timer_recv_sock, SOL_SOCKET, SO_RCVBUF, &new_buff, sizeof(&new_buff));
}

void __init_troll_sock_c(int* troll_sock, struct sockaddr_in* troll_addr) {
    if((*troll_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening troll socket");
        exit(1);
    }
    printf("troll socket initialized \n");
    //Constructing socket name of the troll to send to
    troll_addr->sin_family      = AF_INET;
    troll_addr->sin_port        = htons(TROLL_PORT_CLIENT);
    troll_addr->sin_addr.s_addr = inet_addr(LOCAL_HOST);
}

int is_window_empty(int window[]) {
    for(int i = 0; i < WINDOW_SIZE; i++) {
        if(window[i] != 0xFFFFFFFF) {
            return FALSE;
        }
    }
    return FALSE;
}