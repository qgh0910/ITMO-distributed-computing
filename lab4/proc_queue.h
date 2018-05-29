#ifndef _PROC_QUEUE_H_
#define _PROC_QUEUE_H_

#include "ipc.h"

typedef struct QueueNode {
	local_id proc_id;
	timestamp_t time;
	struct QueueNode *head, *tail, *next;
} QueueNode;

int init_proc_queue (QueueNode **queue);
int enqueue (local_id pid, timestamp_t time, QueueNode *queue);
QueueNode* dequeue (QueueNode *queue);
int clear_queue (QueueNode *queue);

#endif