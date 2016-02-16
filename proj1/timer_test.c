//
// Created by xu on 2/15/16.
//
#include "delta_list.h"

void send_test_message(
        int sock,
        struct sockaddr_in sin_addr,
        time_message time_message_send,
        int seq_num,
        int time,
        int action);

int main(int argc, char** argv) {
    int sock;
    struct sockaddr_in sin_addr;
    struct hostent *hp;
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("UDP socket opening error");
        exit(1);
    }
    hp = gethostbyname(LOCAL_HOST);
    if(hp == 0)
    {
        perror("host unkonwn");
        exit(2);
    }
    bcopy((void *)hp->h_addr, (void *)&sin_addr.sin_addr, hp->h_length);
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port = htons(TIMER_PORT_SERVER);
    printf("PORT IS: %d\n", ntohs(sin_addr.sin_port));

    time_message time_msg_send;

    send_test_message(sock, sin_addr, time_msg_send, 1, 2*1e6, START);
    send_test_message(sock, sin_addr, time_msg_send, 2, 3*1e6, START);
    send_test_message(sock, sin_addr, time_msg_send, 3, 4*1e6, START);
    usleep(5*1e6);
    send_test_message(sock, sin_addr, time_msg_send, 3, 0, CANCEL);
    send_test_message(sock, sin_addr, time_msg_send, 4, 5*1e6, START);
    usleep(5*1e6);
    send_test_message(sock, sin_addr, time_msg_send, 4, 0, CANCEL);
    usleep(5*1e6);
    return 0;
}

void send_test_message(
        int sock,
        struct sockaddr_in sin_addr,
        time_message time_message_send,
        int seq_num,
        int time,
        int action) {
    /* Creat new packet*/
    time_message_send.seq_num = seq_num;
    time_message_send.time = time;
    time_message_send.action = action;


    /* Send msg to timer*/
    if(sendto(
            sock,
            (char *)&time_message_send,
            sizeof(time_message),
            0,
            (struct sockaddr*)&sin_addr,
            sizeof(sin_addr)) < 0) {
        perror("send msg to time error");
        exit(3);
    }
}