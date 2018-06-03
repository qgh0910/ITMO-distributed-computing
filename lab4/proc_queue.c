#include <stdlib.h>
#include "proc_queue.h"

int create_queue(Node **queue){
	*queue = malloc (sizeof **queue);
	if (*queue == NULL)
		return 1;
	else
		return 0;
}

int push_queue (Node *queue, local_id proc_id, timestamp_t time){
	if (queue == NULL) return 1;

	Node* new_node;
	new_node = malloc (sizeof *new_node);
	if (new_node == NULL) return 2;

	new_node->proc_id = proc_id;
	new_node->time = time;
	new_node->next = NULL;

	// if queue is empty => create first node in queue
	if (queue->head == NULL) {
		queue->head = new_node;
		queue->tail = new_node;
		return 0;
	}
	// if queue is not empty => push to queue
	Node* curr_node = queue->head;
	Node* prev_node = NULL;

	do {
		// check requests order comparing by time and proc_id (if times are equal)
		if ((curr_node->time > new_node->time) ||
			(curr_node->time == new_node->time && new_node->proc_id < curr_node->proc_id)) {

			if (prev_node != NULL) prev_node->next = new_node;
			new_node->next = curr_node;
			if (curr_node == queue->head) queue->head = new_node;
			return 0;

		} else {
			prev_node = curr_node;
			curr_node = curr_node->next;
		}
	} while (curr_node != NULL); 

	// if all requests time is less than new_node time => put it in queue tail
	queue->tail->next = new_node;
	queue->tail = new_node;
	return 0;
}

Node* pop_queue (Node *queue){
	if (queue == NULL || queue->head == NULL)
		return NULL;

	Node* result = queue->head;
	if (queue->head == queue->tail) {
		queue->tail = queue->head = NULL;		
	} else {
		queue->head = queue->head->next;
	}

	return result;
}

int destroy_queue (Node *queue){
	if (queue == NULL) return 1;

	Node* curr_node = queue->head;
	Node* tmp_node;
	while (curr_node) {
		tmp_node = curr_node->next;
		free(curr_node);
		curr_node = tmp_node;		
	}
	queue->head = queue->tail = NULL;
	free(queue);

	return 0;
}
