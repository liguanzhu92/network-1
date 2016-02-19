#include "headers/delta_list.h"

node *create_node(int seq_num, long time) {
    node *new_node = NULL;
    new_node = (node *) malloc(sizeof(node));
    if (new_node != NULL) {
        new_node->delta_time = 0;
        new_node->time = time; //default time
        new_node->seq_num = seq_num; //default seq
        new_node->prev = NULL;
        new_node->next = NULL;
    } else {
        perror("ERROR");
    }
    return new_node;
}

linked_list *create_list() {
    linked_list *new_list = NULL;
    new_list = (linked_list *) malloc(sizeof(linked_list));
    if (new_list != NULL) {
        new_list->len = 0;
        new_list->head = NULL;
        new_list->tail = NULL;
    } else {
        perror("ERROR");
    }
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
        insert_node->delta_time = insert_node->time;
        goto success;
    }
    //check the node seq
    for (; temp_seq != temp_head->seq_num; temp_head = temp_head->next) {
        if (temp_head->next == NULL)
            break;
    }

    if (temp_seq == temp_head->seq_num) {
        list->len -= 1;
        goto fail;
    }
    //find the proper location according to the insert_node->time
    //reset temp head
    temp_head = list->head;

    //compare head
    if (temp_time < temp_head->time) {
        insert_node->next = temp_head;
        insert_node->delta_time = temp_time;
        temp_head->prev = insert_node;
        temp_head->delta_time = temp_head->time - insert_node->time;
        list->head = insert_node;
        goto success;
    }
    //compare tail
    if (temp_time >= temp_tail->time) {
        temp_tail->next = insert_node;
        list->tail = insert_node;
        insert_node->prev = temp_tail;
        insert_node->delta_time = temp_time - temp_tail->time;
        goto success;
    }
    //determine the location
    node *temp_node = list->head;
    while (temp_node->time < temp_time)
        temp_node = temp_node->next;
    //time is equal previous one
    if (temp_node->time == temp_time)
        temp_node = temp_node->next;
    //insert the node
    temp_node->prev->next = insert_node;
    insert_node->next = temp_node;
    insert_node->prev = temp_node->prev;
    temp_node->prev = insert_node;
    //update delta_time
    insert_node->delta_time = temp_time - insert_node->prev->time;
    temp_node->delta_time = temp_node->time - temp_time;

    success:
    printf("Successfully inserted node %d\n", insert_node->seq_num);
    return TRUE;
    fail:
    printf("Node %d is existed, insertion failed!\n", temp_seq);
    return FALSE;
}

int cancel_node(linked_list *list, int seq_num) {
    // search the seq_num node
    node *temp_node = list->head;
    for (; temp_node != NULL; temp_node = temp_node->next) {
        if (temp_node->seq_num == seq_num) {
            break;
        }
    }
    if (temp_node == NULL) {
        goto fail;
    }

    list->len -= 1;
    // cancel the only node in list
    if ((temp_node->prev == NULL) && (temp_node->next == NULL)) {
        list->head = NULL;
        list->tail = NULL;
        goto success;
    } else {
        if (temp_node->prev == NULL) { //cancel head node
            //update pointer
            list->head = temp_node->next;
            temp_node->next->prev = NULL;
            temp_node->next->delta_time = temp_node->next->time;
        } else if (temp_node->next == NULL) { //cancel tail node
            list->tail = temp_node->prev;
            temp_node->prev->next = NULL;
        } else {
            temp_node->prev->next = temp_node->next;
            temp_node->next->prev = temp_node->prev;
        }

        success:
        printf("Successfully deleted node %d\n", temp_node->seq_num);
        free(temp_node);
        return TRUE;
        fail:
        printf("Node %d does not exist, cancellation failed!\n", seq_num);
        return FALSE;
    }
}

int remove_node(linked_list* list, node *remove_node) {
    if (remove_node != NULL) {
        list->head = remove_node->next;
        if(remove_node->next != NULL) {
            remove_node->next->delta_time = remove_node->next->time;
        }
        free(remove_node);
    } else {
        perror("remove_node error");
    }
    return TRUE;
}

int print_list(linked_list *list) {
    node *print_node = list->head;
    if (print_node == NULL) {
        printf("- empty list -\n");
        return 0;
    }
    long print_time;
    int seq_num;
    printf("-----------ascending-----------\n");
    for (int i = 0; i < list->len; ++i) {
        seq_num = print_node->seq_num;
        print_time = print_node->time ;
        printf("sequence number: %d, time: %ld\xC2\xB5s\n", seq_num, print_time);
        print_node = print_node->next;
    }
    printf("-----------descending----------\n");
    print_node = list->tail;
    for (int i = 0; i < list->len; ++i) {
        seq_num = print_node->seq_num;
        print_time = print_node->time;
        printf("sequence number: %d, time: %ld\xC2\xB5s\n", seq_num, print_time);
        print_node = print_node->prev;
    }
    printf("*******************************\n");
    return 0;
}

int is_expired(linked_list *list) {
    if (list == NULL || list->head == NULL)
        return FALSE;
    if (list->head->time <= 0)
        return TRUE;
    return FALSE;
}

void update_list(linked_list* list) {
    node* temp_tail = list->tail;
    // update list
    for (; temp_tail != NULL; temp_tail =  temp_tail->prev) {
        if (temp_tail->prev != NULL)
            temp_tail->time = temp_tail->time - (list->head->delta_time - list->head->time);
        else
            temp_tail->delta_time = temp_tail->time;
    }
}