#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>

#include "io.h"
#include "pa1.h"

#include "process.h"

// all child process work including sync and useful work
int child_process(IO* io, local_id proc_id) {
	// set IO struct for the process
	IO this_process = {
		.proc_id = proc_id,
		.proc_number = io->proc_number,
		.channels = io->channels,
		.events_log_stream = io->events_log_stream,
		.pipes_log_stream = io->pipes_log_stream
	};

	// set for messages and sync
	char payload[MAX_PAYLOAD_LEN];
	int payload_len = sprintf(payload, log_started_fmt, proc_id, getpid(), getppid());
	MessageType type = STARTED;
	timestamp_t t = 0;

	fputs(payload, this_process.events_log_stream);
	synchronize_with_others(payload_len, type, t, payload, &this_process);
	fprintf(this_process.events_log_stream, log_received_all_started_fmt, proc_id);
	do_child_work();

	// set for msg and sync
	payload_len = sprintf(payload, log_done_fmt, proc_id);
	fputs(payload, this_process.events_log_stream);
	type = DONE;

	synchronize_with_others(payload_len, type, t, payload, &this_process);
	fprintf(this_process.events_log_stream, log_received_all_done_fmt, proc_id);
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

int do_child_work() { return 0;}
