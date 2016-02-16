//
// Created by xu on 2/14/16.
//
#include "delta_list.h"

struct timeval timeout = {
        0,
        (__suseconds_t) (1 * 1e5),
};

int main()
{
    //int sock_timer;
    int sock_timer_recv;
    int sock_timer_send;
    time_message time_msg_recv;
    time_message time_msg_send;
    struct sockaddr_in timer_recv_addr;
    struct sockaddr_in timer_send_addr;
    int new_buf_size = SOCK_BUF_SIZE;

    linked_list *time_list;
    if((sock_timer_recv = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("opening datagram socket for recv from tcpd_m1");
    }
    timer_recv_addr.sin_family = AF_INET;
    timer_recv_addr.sin_port = htons(TIMER_PORT_SERVER);
    timer_recv_addr.sin_addr.s_addr = INADDR_ANY;


    if(bind(sock_timer_recv, (struct sockaddr *)&timer_recv_addr, sizeof(timer_recv_addr)) < 0)
    {
        perror("Timer send to tcpd socket Bind failed");
        exit(ENOTCONN);
    }
    setsockopt(sock_timer_recv, SOL_SOCKET, SO_RCVBUF, &new_buf_size, sizeof(&new_buf_size));

    if((sock_timer_send = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("sock_timer_send sock failed )");
    }
    timer_send_addr.sin_family = AF_INET;
    timer_send_addr.sin_port = htons(TIMER_PORT_CLIENT);
    timer_send_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);



    time_list = create_list();
    fd_set fd_read_set;
    struct timeval last_sleep, current_time;
    struct timezone time_zone;
    int len = sizeof(timer_recv_addr);
    int MAXFD = sock_timer_recv + 1;
    long delta_time;
    FD_ZERO(&fd_read_set);
    FD_SET(sock_timer_recv, &fd_read_set);


    while(1) {
        gettimeofday(&last_sleep, &time_zone);
        /* Receive msg from socket, block here if no msg available */
        if (select(MAXFD, &fd_read_set, NULL, NULL, &timeout) < 0) {
            perror("select error");
            exit(0);
        }
        gettimeofday(&current_time, &time_zone);

        /* delta_time: microseconds elapsed */
        delta_time = 1e6 * (current_time.tv_sec - last_sleep.tv_sec) + (current_time.tv_usec - last_sleep.tv_usec);

        if (time_list->head != NULL)
            time_list->head->time -= delta_time;

        if (FD_ISSET(sock_timer_recv, &fd_read_set)) {
            if (recvfrom(
                    sock_timer_recv,
                    (char *) &time_msg_recv,
                    sizeof(time_msg_recv),
                    0,
                    (struct sockaddr *) &timer_recv_addr,
                    &len) < 0) {
                perror("recvfrom TCPD_M2 error");
                exit(0);
            }
            //print_list(time_list);
            if (time_msg_recv.action == CANCEL) { /* Cancel node*/
                printf("\ncancel node: %d\n", time_msg_recv.seq_num);
                printf("before cancel:\n");
                print_list(time_list);
                cancel_node(time_list, time_msg_recv.seq_num);
                printf("after cancel:\n");
                print_list(time_list);
                printf("-------------------------\n");
            } else if (time_msg_recv.action == START) { /* Add node for timing */
                printf("\nstart node: %d; time: %d\n", time_msg_recv.seq_num, time_msg_recv.time);
                printf("before add:\n");
                print_list(time_list);
                node *new_node = create_node(time_msg_recv.seq_num, time_msg_recv.time);
                insert_node(time_list, new_node);
                printf("after add:\n");
                print_list(time_list);
                printf("-------------------------\n");
            }
        }
//                timeout.tv_usec = 1*1e5;
        while (is_expired(time_list)) {
            node *expired_node, *ptr;
            long dtime;
            printf("-------------\n");
            printf("Have something expried:\n");
            print_list(time_list);
            printf("-------------\n");

            for (ptr = time_list->head; ptr != NULL;) {
                if (ptr->time > 0)
                    break;
                time_list->head = ptr->next;
                if (time_list->head != NULL)
                    time_list->head->prev = NULL;
                time_list->len--;

                time_msg_send.seq_num = ptr->seq_num;
                time_msg_send.action = EXPIRED;
                time_msg_send.time = 0;
                //printf("\nBEGIN REMOVE NODE\n");
                //cancel_node(time_list,expire_node);
                if (time_list->len <= 0) {
                    time_list->head = NULL;
                }

                send_again:
                if (sendto(
                        sock_timer_send,
                        (void *) &time_msg_send,
                        sizeof(time_msg_send),
                        0,
                        (struct sockaddr *) &timer_send_addr,
                        sizeof(struct sockaddr_in)) < 0) {
                    if (errno == EINTR) goto send_again;
                    perror("\nTIMER SEND ERROR\n");
                    exit(1);
                }
                expired_node = ptr;
                dtime = ptr->time;
                ptr = ptr->next;
                remove_node(expired_node);

            }

            if (time_list->head == NULL) {
                time_list->tail = NULL;
            } else {
                time_list->head->time += dtime;
            }
        }

        if (time_list->len != 0) {
            printf("--------------------------\n");
            printf("After removed expired nodes: \n");
            print_list(time_list);
            printf("--------------------------\n");
            if (time_list->head == NULL) {
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
            } else {
                timeout.tv_sec = time_list->head->time / 1000000;
                timeout.tv_usec = time_list->head->time % 1000000;
            }
        }

        FD_ZERO(&fd_read_set);
        FD_SET(sock_timer_recv, &fd_read_set);

    }
    close(sock_timer_recv);
    close(sock_timer_send);
    return 0;
}

