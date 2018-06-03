#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "process.h"
#include "ipc.h"
#include "io.h"
#include "util.h"
#include "main.h"
#include "proc_queue.h"

#define BUFF_SIZE 4096

int wait_all_messages (IO* io, MessageType type) {
	if (io == NULL) return 1;
	Message msg = { {0} };

	int proc_counter = io->proc_number;
	while (proc_counter) {
        while(receive_any((void*)io, &msg) < 0);
        if (type == msg.s_header.s_type) {
            proc_counter--;
            set_actual_time(msg.s_header.s_local_time);
        }
    }

    return 0;
}

void syncronize_with_others (IO* io, MessageType type) {
	if (io == NULL) return;

	Message msg;
    increase_time();

    // send message to all processes
    msg.s_header = (MessageHeader) {
            .s_magic       = MESSAGE_MAGIC,
            .s_payload_len = 0,
            .s_type        = type,
            .s_local_time  = get_lamport_time()
    };
    send_multicast((void*)io, &msg);

    // wait reply from all processes
    int proc_counter = io->working_proc_number - 1;
    while (proc_counter) {
        while(receive_any((void*)io, &msg) < 0);
        set_actual_time(msg.s_header.s_local_time);
        if (type == msg.s_header.s_type) {
            proc_counter--;
        }
    }
}

int child_work(IO* io) {
	if (io == NULL) return 1;

	if (io->is_mutexl) request_cs(io);

    // do "useful" work
	char buff[BUFF_SIZE];
	int n = io->proc_id * 5;
	for (int i = 1; i <= n; i++ ) {
		snprintf(buff, sizeof(buff), log_loop_operation_fmt, io->proc_id, i, n);
		print(buff);
	}

	if (io->is_mutexl) release_cs(io);

	return 0;
}

int child_process(IO* io, local_id proc_id) {
	if (io == NULL) return 1;

    char payload[MAX_PAYLOAD_LEN];
    size_t len;
	printf("Started child process with id %d \n", proc_id);

	// create child process IO 
    Node* queue;
    create_queue(&queue);
	IO this_process = {
		.proc_id = proc_id,
		.proc_number = io->proc_number,
		.channels = io->channels,
		.events_log_stream = io->events_log_stream,
		.pipes_log_stream = io->pipes_log_stream,		
		.working_proc_number = io->proc_number,
		.is_mutexl = io->is_mutexl,
        .proc_queue = queue
    };    

	// close non related pipes
	close_non_related_fd(&this_process, proc_id);

	len = sprintf(payload, log_started_fmt, get_lamport_time(), proc_id, getpid(), getppid(), 0);
    fputs(payload, this_process.events_log_stream); 

	// send STARTED msg to others, receive reply 
    syncronize_with_others(&this_process, STARTED);
    fprintf(this_process.events_log_stream, log_received_all_started_fmt, get_lamport_time(), proc_id);

	// do child work
    child_work(&this_process);

	// send DONE msg to others, receive all DONE msgs, count completed processes
	len = sprintf(payload, log_done_fmt, get_lamport_time(), proc_id, 0);
    fputs(payload, this_process.events_log_stream);
    
    syncronize_with_others(&this_process, DONE);
    fprintf(this_process.events_log_stream, log_received_all_done_fmt, get_lamport_time(), proc_id);

    return 0;
}

int parent_process(IO* io) {	
	close_non_related_fd(io, PARENT_ID);

	// wait children processes
    io->proc_id = PARENT_ID;
    wait_all_messages(io, STARTED);
    fprintf(io->events_log_stream, log_received_all_started_fmt, get_lamport_time(), PARENT_ID);
    wait_all_messages(io, DONE);
    fprintf(io->events_log_stream, log_received_all_done_fmt, get_lamport_time(), PARENT_ID);

    while(wait(NULL) > 0);

    fclose(io->pipes_log_stream);
    fclose(io->events_log_stream);

    return 0;
}

