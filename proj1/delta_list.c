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
    node *temp_head = list->head;
    node *temp_tail = list->tail;
    long temp_time = insert_node->time;
    int temp_seq = insert_node->seq_num;
    //increase list length
    list->len += 1;
    //if the list is empty, insert head
    if (list->head == NULL) {
        list->head = insert_node;
        list->tail = insert_node;
        return TRUE;
    }
    //check the node seq
    for (; temp_seq != temp_head->seq_num ; temp_head = temp_head->next) {
        if (temp_head->next==NULL)
            break;
    }

    if (temp_seq == temp_head->seq_num ){
        printf("this node has existed\n");
        list->len -= 1;
        return FALSE;
    }
    //find the proper location according to the insert_node->time
    //reset temp head
    temp_head = list->head;

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

    //insert the node
    temp_node->prev->next = insert_node;
    insert_node->next = temp_node;
    insert_node->prev = temp_node->prev;
    temp_node->prev = insert_node;
    //update time_left
    insert_node->time_left = temp_time - insert_node->prev->time;
    temp_node->time_left= temp_node->time - temp_time;

    printf("Successfully insert node %d\n", insert_node->seq_num);

    return TRUE;
}

int cancel_node(linked_list *list, int seq_num) {
    // search the seq_num node
    node *temp_node = list->head;
    for (;temp_node != NULL; temp_node = temp_node->next) {
        if (temp_node->seq_num == seq_num)
            break;
    }
    if(temp_node == NULL){
        printf("this node does not exist\n");
        return FALSE;
    }

    list->len -= 1;
    if ((temp_node->prev==NULL )&& (temp_node->next== NULL)){
        list->head = NULL;
        list->tail = NULL;
        free(temp_node);
        return  TRUE;
    }else{
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
    } else {
        perror("remove_node error");
    }
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
    return 0;
}

int is_expired(linked_list *list) {
    if (list == NULL || list->head == NULL)
        return FALSE;
    if (list->head->time <= 0)
        return TRUE;
    return FALSE;
}