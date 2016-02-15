#include <delta_list.h>


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

node* create_node(int seq_num, long time)
{
   	node *new_node = NULL;
   	new_node = (node*)malloc(sizeof(node));
   	if (new_node != NULL)
   	{
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

linked_list* create_list()
{
	linked_list *new_list = NULL;
	new_list = (linked_list*)malloc(sizeof(linked_list));
	if (new_list != NULL)
	{
		new_list->len = 0;
		new_list->head = NULL;
		new_list->tail = NULL;
	}
	else
		perror("ERROR");
	return new_list;
}

int insert_node(linked_list *list, node *insert_node)
{
	//increase list length
	list->len += 1;
	node *temphead = list->head;
	node *temptail = list->tail;
	long temptime = insert_node->time;
	//if the list is empty, insert head
	if (list->head == NULL)
	{
		list->head = insert_node;
		list->tail = insert_node;
		return TRUE;
	}
	//find the proper location accroding to the insert_node->time
	//compare head
	if (temptime < temphead->time)
	{
		insert_node->next = temphead;
		insert_node->time_left = temptime;
		temphead ->prev = insert_node;
		temphead ->time_left = temphead->time - insert_node->time;
		list->head = insert_node;
		return TRUE;
	}
	//compare tail
	if (temptime > temptail->time)
	{ 
		temptail->next = insert_node;
		list->tail = insert_node;
		insert_node->prev = temptail;
		insert_node->time_left = temptime - temptail->time;
		return TRUE;
	}
	//determine the location
	while(tempnode->time < temptime)
		tempnode = tempnode->next；
	
	if (tempnode->time != temptime)
	{
		//insert the node
		insert_node->next = tempnode->next;
		tempnode->next->prev = insert_node;
		insert_node->prev = tempnode;
		tempnode->next = insert_node;
		//update time_left
		tempnode->next->time_left = tempnode->next->time - temptime;
		insert_node->time_left = temptime -  tempnode->time;
		return TRUE;		
	}
	printf("Successfully insert node %d\n", insert_node->seq_num);
	if (tempnode->time == temptime)
	{
		printf("This node has existed");
		return FALSE;		
	}
}

int cancel_node(linked_list *list, int seq_num)
{
	// search the seq_num node
	node *tempnode = list->head; 
	while(tempnode->seq_num != seq_num)
		tempnode = tempnode->next;
	list->len -= 1;
	if (remove_node == NULL)
	{
		perror("does not exist this node");
		return FALSE;
		list->len += 1;
	}
	else
	{
		//head
		if (remove_node->prev == NULL)
		{
			//update pointer
			list->head = remove_node->next;
			remove_node->next->prev = NULL;
			remove_node->next->time_left = remove_node->next->time;
		}
		//tail
		else if (remove_node->next == NULL)
		{
			list->tail = remove_node->prev;
			remove_node->prev->next = NULL;
		}
		else
		{
			remove_node->prev->next = remove_node->next;
			remove_node->next->prev = remove_node->prev;
			
		}
		printf("Successfully delete node %d\n",remove_node->seq_num);
		free(remove_node);
		return TRUE;	
	}	
}

int remove_node(node *remove_node)
{
    if(remove_node != NULL)
    {
    	free(remove_node);    
    }
    else
    	perror("remove_node error");
    return TRUE;
}

int print_list(linked_list *list)
{
	node *printnode = list->head;
	long time;
	int seq_num;	
	printf("-------Ascending-------");
	for (int i = 0; i < list->len; ++i)
	{
		seq_num = printnode->seq_num;
		time = printnode->time;
		printf("sequence number:%d, time:%d\n",seq_num,time );
		printnode = printnode->next;
	}
	printf("-------descending-------");
	printnode = list->tail;
	for (int i = 0; i < list->len; --i)
	{
		seq_num = printnode->seq_num;
		time = printnode->time;
		printf("sequence number:%d, time:%d\n",seq_num,time );
		printnode = printnode->prev;
	}
}

int is_expired(linked_list *list)
{
	if (list == NULL || list->head == NULL)
		return FALSE;
	if (list->head->time <= 0)
		return TRUE;
	return FALSE;
}