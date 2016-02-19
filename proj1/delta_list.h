//
// Created by xu on 2/14/16.
//

#ifndef DELTA_LIST_H
#define DELTA_LIST_H

#include <sys/time.h>
#include <errno.h>
#include "tcpd.h"

#define TRUE 1
#define FALSE 0
#define CANCEL 0
#define START 1
#define EXPIRED 2

#define DEFAULT_TIMEOUT 10

typedef struct node {
    long time_left;
    long time;
    int seq_num;
    struct node *prev;
    struct node *next;
} node;

typedef struct linked_list {
    int len;
    node *head; 
    node *tail;
} linked_list;

/* Packet data struct */
typedef struct time_message {
    int seq_num;
    int action;
    long time;
} time_message;

node* create_node(int seq_num, long time);
linked_list* create_list();
int insert_node(linked_list *list, node *insert_node);
int cancel_node(linked_list *list, int seq_num);
int remove_node(node *remove_node);
int print_list(linked_list *list);
int is_expired(linked_list *list);

#endif