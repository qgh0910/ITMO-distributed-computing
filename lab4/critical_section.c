#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "main.h"
#include "pa2345.h"
#include "proc_queue.h"

int request_cs(const void * self) {
	if (self == NULL) return 1;

    IO* io = (IO*)self;
    if (io == NULL) return 2;
    
    int last_sender_id;
    increase_time();

    // send critical section request to all processes
    Message msg;
    msg.s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_type  = CS_REQUEST,
        .s_local_time = get_lamport_time(),
        .s_payload_len = 0
    };
    send_multicast((void*)self, &msg);

    // insert request to process own lamport queue
    push_queue (io->proc_queue, io->proc_id, get_lamport_time());

    int repliers_count = io->proc_number - 1;

    // wait for all processes reply or for this process request becoming first in cs queue
    while (repliers_count != 0 || (io->proc_queue->head != NULL && io->proc_queue->head->proc_id != io->proc_id)) {
        while ((last_sender_id = receive_any((void*)self, &msg)) < 0);
        set_actual_time(msg.s_header.s_local_time);

        switch (msg.s_header.s_type) {
            case CS_REQUEST:
                printf("%d: process %d got request from %d\n", get_lamport_time(), io->proc_id, last_sender_id);
                push_queue (io->proc_queue, last_sender_id, msg.s_header.s_local_time);
                increase_time();
                msg.s_header.s_type = CS_REPLY;
                msg.s_header.s_local_time = get_lamport_time();
                send(io, last_sender_id, &msg);
                break;

            case CS_REPLY:
                printf("%d: process %d got reply from %d\n", get_lamport_time(), io->proc_id, last_sender_id);
                repliers_count -= 1;
                break;

            case CS_RELEASE:
                printf("%d: process %d got release from %d\n", get_lamport_time(), io->proc_id, last_sender_id);
                Node* node = pop_queue (io->proc_queue);
                free(node);
                break;

            case DONE:
                printf("%d: process %d got DONE from %d\n", get_lamport_time(), io->proc_id, last_sender_id);
                io->working_proc_number -= 1;
                break;

            default:
                break;
        }
    }
    
    return 0;
}

int release_cs(const void * self) {
    if (self == NULL) return 1;

    IO* io = (IO*)self;
    if (io == NULL) return 2;

    // remove first request from queue
    free (pop_queue(io->proc_queue));
    
    Message msg;
    msg.s_header = (MessageHeader) {
        .s_magic = MESSAGE_MAGIC,
        .s_type  = CS_RELEASE,
        .s_local_time = get_lamport_time(),
        .s_payload_len = 0
    };

    increase_time();
    send_multicast((void*)self, &msg);

	return 0;	
}
