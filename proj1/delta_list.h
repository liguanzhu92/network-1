//
// Created by xu on 2/14/16.
//
#include <sys/time.h>
#include <errno.h>
#include "tcpd.h"

#define TRUE 1
#define FALSE 0
#define CANCEL 0
#define START 1
#define EXPIRE 2

typedef struct node {
    int time_left;
    long time;
    int seq_num;
    struct node *prev;
    struct node *next;
} node;

typedef struct link_list {
    int len;
    node *head;
    node *tail;
} link_list;

/* Packet data struct */
typedef struct time_message {
    int seq_num;
    int action;
    long time;
} time_message;

node* create_node(int seq, long time);
link_list* create_list();
int insert_node(link_list *list, node *insert_node);
int cancel_node(link_list *list, int seq);
int remove_node(node *remove_node);
int print_list(link_list *list);
int is_expired(link_list *list);
