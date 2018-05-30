#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>

#include "io.h"
#include "pa2345.h"
#include "process.h"
#include "main.h"
#include "util.h"

// all child process work including sync and useful work
int child_process(IO* io, local_id proc_id) {
	// set IO struct for the process
	IO this_process = {
		.proc_id = proc_id,
		.proc_number = io->proc_number,
		.channels = io->channels,
		.events_log_stream = io->events_log_stream,
		.pipes_log_stream = io->pipes_log_stream,
		.completed_proc_counter = io->completed_proc_counter,
		.is_mutexl = io->is_mutexl
	};
	
	increase_time();
	// set for messages and sync
	char payload[MAX_PAYLOAD_LEN];
	int payload_len = sprintf(payload, log_started_fmt, get_lamport_time(),
							  proc_id, getpid(), getppid(),
							  0);
	MessageType type = STARTED;
	timestamp_t t = 0; // used only for synchronization

	// sync STARTED
	fputs(payload, this_process.events_log_stream);
	synchronize_with_others(payload_len, type, t, payload, &this_process);
	fprintf(this_process.events_log_stream, log_received_all_started_fmt,
			get_lamport_time(), proc_id);

	// DO WORK
	do_child_work(&this_process);

	// DONE
	payload_len = sprintf(payload, log_done_fmt, get_lamport_time(), proc_id, 0);
	fputs(payload, this_process.events_log_stream);
	// fclose(this_process.events_log_stream);
	// fclose(this_process.pipes_log_stream);
	increase_time();
	type = DONE;

	// sync DONE
	Message msg;
	msg.s_header = (MessageHeader) {
		.s_magic = MESSAGE_MAGIC,
		.s_payload_len = strlen(msg.s_payload),
		.s_type = DONE,
		.s_local_time = get_lamport_time()
	};	

	int rc = send_multicast(&this_process, &msg);
	if( rc < 0 ){
		return 1;
	}

	while (this_process.completed_proc_counter < this_process.proc_number - 1){
		rc = receive_any(&this_process, &msg);
		if( rc != 0 ) return 1;
		set_actual_time(msg.s_header.s_local_time);
		increase_time();
		if( msg.s_header.s_type == DONE ) this_process.completed_proc_counter++;
	}
	fprintf(this_process.events_log_stream, log_received_all_done_fmt,
		get_lamport_time(), proc_id);
	
	return 0;
}

int synchronize_with_others(
	uint16_t payload_len,
	MessageType type,
	timestamp_t local_time,
	char* payload,
	IO* proc)
{
	// fullfill message
	Message msg;
	msg.s_header = (MessageHeader) {
		.s_magic = MESSAGE_MAGIC,
		.s_payload_len = payload_len,
		.s_type = type,
		.s_local_time = local_time
	};
	strncpy(msg.s_payload, payload, payload_len);

	send_multicast((void*)proc, (const Message *)&msg);
	// wait for messages from others
	// it's kinda "stub" which will prevent you to move on
	for (size_t i = 1; i <= proc->proc_number; i++)
		if (i == proc->proc_id)
			continue;
		else
			while (receive((void*)proc, i, &msg) != 0);
	return 0;
}

#define BUFF_SIZE 4096
int do_child_work(IO* io)
{
	if (io->is_mutexl)
		request_cs(io);

	char buff[BUFF_SIZE];
	int n = io->proc_id * 5;
	for (int i = 1; i <= n; i++ ) {
		snprintf(buff, sizeof(buff), log_loop_operation_fmt, io->proc_id, i, n);
		print(buff);
	}

	if (io->is_mutexl)
		release_cs(io);

	return 0;
}
