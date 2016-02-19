//
// Created by xu on 2/15/16.
//
#include "delta_list.h"

int sock;
struct sockaddr_in sin_addr;
time_message time_message_send;

void _send_test_message(int seq_num, long time, int action);
void start_timer(float time_in_sec, int seq_num);
void cancel_timer(int seq_num);

int main(int argc, char** argv) {
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

    start_timer(4, 55);
    start_timer(2, 1);//insert (first node)
    cancel_timer(1);//cancel first
    cancel_timer(50);//cancel not existed
    start_timer(4, 55);//add same seq_num
    start_timer(3, 2);
    start_timer(4, 3);
    sleep(5);
    cancel_timer(3);//remove last
    start_timer(5, 4);
    sleep(5);
    cancel_timer(4);
    sleep(5);
    
    /*start_timer(10.0,2);
    start_timer(30.0,3);
    sleep(5);
    cancel_timer(2);
    start_timer(20.0,4);
    sleep(5);
    start_timer(18.0,5);
    cancel_timer(4);
    cancel_timer(8);*/
    return 0;
}

void start_timer(float time_in_sec, int seq_num) {
    //TODO may have overflow here
    _send_test_message(seq_num, time_in_sec * (long)1e6, START);
}

void cancel_timer(int seq_num) {
    _send_test_message(seq_num, 0, CANCEL);
}

void _send_test_message(int seq_num, long time, int action) {
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
