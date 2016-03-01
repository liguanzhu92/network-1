#include "headers/delta_list.h"

node *create_node(int seq_num, long delta_time) {
    node *new_node = NULL;
    new_node = (node *) malloc(sizeof(node));
    if (new_node != NULL) {
        new_node->delta_time = delta_time;
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
    long time = 0;
    int temp_seq = insert_node->seq_num;
    //increase list length
    list->len += 1;
    //if the list is empty, insert head
    if (list->head == NULL) {
        list->head = insert_node;
        list->tail = insert_node;
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
    //find the proper location according to the insert_node->delta_time
    //reset temp head
    temp_head = list->head;

    //compare head
    if (insert_node->delta_time < temp_head->delta_time) {
        insert_node->next = temp_head;
        temp_head->prev = insert_node;
        temp_head->delta_time = temp_head->delta_time - insert_node->delta_time;
        list->head = insert_node;
        goto success;
    }
        //determine the location
    node *temp_node = list->head;
    node *temp_prev;
    long prev_time = 0;
    for (; insert_node->delta_time > time; temp_node = temp_node->next) {
        prev_time = time;

        if(temp_node == NULL)
            break;
        temp_prev = temp_node;
        time +=temp_prev->delta_time;
    }
    //insert the node
    temp_prev->next = insert_node;
    insert_node->prev = temp_prev;
    //update time
    insert_node->delta_time -= prev_time;
    if (temp_node == NULL){ // insert tail
        insert_node->next == NULL;
        goto success;
    }
    insert_node->next = temp_node;
    temp_node->prev = insert_node;
    temp_node->delta_time -= insert_node->delta_time;

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
            temp_node->next->delta_time += temp_node->delta_time;
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
            remove_node->next->delta_time += remove_node->delta_time;
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
        print_time = print_node->delta_time ;
        if(i != list->len -1) {
            printf("%d@: %ld\xC2\xB5s <=> ", seq_num, print_time);
        } else {
            printf("%d, @: %ld\xC2\xB5s\n", seq_num, print_time);
        }
        print_node = print_node->next;
    }
/*    printf("-----------descending----------\n");
    print_node = list->tail;
    for (int i = 0; i < list->len; ++i) {
        seq_num = print_node->seq_num;
        print_time = print_node->delta_time;
        if(i != list->len -1) {
            printf("%d@: %ld\xC2\xB5s -> ", seq_num, print_time);
        } else {
            printf("%d, @: %ld\xC2\xB5s\n", seq_num, print_time);
        }
        print_node = print_node->prev;
    }*/
    printf("*******************************\n");
    return 0;
}

int is_expired(linked_list *list) {
    if (list == NULL || list->head == NULL)
        return FALSE;
    if (list->head->delta_time <= 0)
        return TRUE;
    return FALSE;
}
