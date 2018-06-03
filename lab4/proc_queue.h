#ifndef _PROC_QUEUE_H_
#define _PROC_QUEUE_H_
#include "ipc.h"

typedef struct Node
{
	local_id proc_id;
	timestamp_t time;
	struct Node *head, *tail, *next;	
} Node;

int create_queue(Node **queue);
int push_queue (Node *queue, local_id proc_id, timestamp_t time);
Node* pop_queue (Node *queue);
int destroy_queue (Node *queue);

#endif

