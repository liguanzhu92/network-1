#include "delta_list.h"


/*typedef struct node {
    int time_left;//additional time needed beyond the expiry of the previous event
    long time;//started expire
    int seq_num; // port number
    struct node *prev;
    struct node *next;
} node;

typedef struct linked_list {
    int len;
    node *head; 
    node *tail;
} linked_list;*/

node *create_node(int seq_num, long time) {
    node *new_node = NULL;
    new_node = (node *) malloc(sizeof(node));
    if (new_node != NULL) {
        new_node->time_left = 0;
        new_node->time = time; //default time
        new_node->seq_num = seq_num; //default seq
        new_node->prev = NULL;
        new_node->next = NULL;
    }
    else
        perror("ERROR");
    return new_node;
}

linked_list *create_list() {
    linked_list *new_list = NULL;
    new_list = (linked_list *) malloc(sizeof(linked_list));
    if (new_list != NULL) {
        new_list->len = 0;
        new_list->head = NULL;
        new_list->tail = NULL;
    }
    else
        perror("ERROR");
    return new_list;
}

int insert_node(linked_list *list, node *insert_node) {
    //increase list length
    list->len += 1;
    node *temp_head = list->head;
    node *temp_tail = list->tail;
    long temp_time = insert_node->time;
    //if the list is empty, insert head
    if (list->head == NULL) {
        list->head = insert_node;
        list->tail = insert_node;
        return TRUE;
    }
    //find the proper location accroding to the insert_node->time
    //compare head
    if (temp_time < temp_head->time) {
        insert_node->next = temp_head;
        insert_node->time_left = temp_time;
        temp_head->prev = insert_node;
        temp_head->time_left = temp_head->time - insert_node->time;
        list->head = insert_node;
        return TRUE;
    }
    //compare tail
    if (temp_time > temp_tail->time) {
        temp_tail->next = insert_node;
        list->tail = insert_node;
        insert_node->prev = temp_tail;
        insert_node->time_left = temp_time - temp_tail->time;
        return TRUE;
    }
    //determine the location
    node *temp_node = list->head;
    while (temp_node->time < temp_time)
        temp_node = temp_node->next;

    if (temp_node->time != temp_time) {
        //insert the node
        insert_node->next = temp_node->next;
        temp_node->next->prev = insert_node;
        insert_node->prev = temp_node;
        temp_node->next = insert_node;
        //update time_left
        temp_node->next->time_left = temp_node->next->time - temp_time;
        insert_node->time_left = temp_time - temp_node->time;
        return TRUE;
    }
    printf("Successfully insert node %d\n", insert_node->seq_num);
    if (temp_node->time == temp_time) {
        printf("This node has existed");
        return FALSE;
    }
}

int cancel_node(linked_list *list, int seq_num) {
    // search the seq_num node
    node *temp_node = list->head;
    while (temp_node->seq_num != seq_num)
        temp_node = temp_node->next;
    list->len -= 1;
    if (temp_node == NULL) {
        perror("does not exist this node");
        list->len += 1;
        return FALSE;

    } else {
        //head
        if (temp_node->prev == NULL) {
            //update pointer
            list->head = temp_node->next;
            temp_node->next->prev = NULL;
            temp_node->next->time_left = temp_node->next->time;
        }
            //tail
        else if (temp_node->next == NULL) {
            list->tail = temp_node->prev;
            temp_node->prev->next = NULL;
        }
        else {
            temp_node->prev->next = temp_node->next;
            temp_node->next->prev = temp_node->prev;

        }
        printf("Successfully delete node %d\n", temp_node->seq_num);
        free(temp_node);
        return TRUE;
    }
}

int remove_node(node *remove_node) {
    if (remove_node != NULL) {
        free(remove_node);
    }
    else
        perror("remove_node error");
    return TRUE;
}

int print_list(linked_list *list) {
    node *print_node = list->head;
    if(print_node == NULL) {
        printf("There is no node in the list.\n");
        return 0;
    }
    long time;
    int seq_num;
    printf("-------Ascending-------\n");
    for (int i = 0; i < list->len; ++i) {
        seq_num = print_node->seq_num;
        time = print_node->time;
        printf("sequence number:%d, time:%ld\n", seq_num, time);
        print_node = print_node->next;
    }
    printf("-------descending-------\n");
    print_node = list->tail;
    for (int i = 0; i < list->len; ++i) {
        seq_num = print_node->seq_num;
        time = print_node->time;
        printf("sequence number:%d, time:%ld\n", seq_num, time);
        print_node = print_node->prev;
    }
}

int is_expired(linked_list *list) {
    if (list == NULL || list->head == NULL)
        return FALSE;
    if (list->head->time <= 0)
        return TRUE;
    return FALSE;
}