#include <stdio.h>
#include "util.h"
#include "main.h"
#include "pa2345.h"
#include "proc_queue.h"

static QueueNode* queue = NULL;

int request_cs(const void * self) {
	if (IO == NULL) return 1;

	increase_time();
	IO* io = (IO*) self;
	if (queue == NULL) init_proc_queue();

	Message msg;
	memset(msg, 0, sizeof *msg);
	msg.s_header = (MessageHeader) {
		.s_magic = MESSAGE_MAGIC,
		.s_type = CS_REQUEST,
		.s_local_time = get_lamport_time();
		.s_payload_len = 0;
	}

	enqueue(io->proc_id, get_lamport_time(), queue);
	int res = send_multicast (io, &msg);
	if (res != 0) return 2;

	int repliers_count = io->proc_number - 2;	// all processes except self and parent must reply
	do {
		res = receive_any (io, &msg);
		if (res != 0) return 3;
		set_actual_time();
		increase_time();

		switch (msg.s_header.s_type) {
			// add request to process queue and reply sender
			case CS_REQUEST: 
				// TODO: check proc_id: mb its necessary to store last_msg_pid?..
				enqueue(msg.)
				break;

			// received reply from process => decrement counter
			case CS_REPLY:
				repliers_count -= 1;
				break;

			case CS_RELEASE:
				free(dequeue(queue));
				break;

			case DONE:
				io->completed_proc_counter += 1;
				break;

			default:
				break;	
		}
	}
}

int release_cs(const void * self) {
	
}
