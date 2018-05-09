#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "io.h"
#include "util.h"
#include "common.h"
#include "ipc.h"
#include "pa1.h"

int events_log_fd = -1;
int pipes_log_fd = -1;

int open_log_streams (IO* io) {
	if (io == NULL)
		return -1;

	io->events_log_stream = fopen(events_log, "w+");
	if (io->events_log_stream == NULL)
		return -2;

	io->pipes_log_stream = fopen(pipes_log, "w");
	if (io->pipes_log_stream == NULL)
		return -3;

	return 0;
}

int close_log_streams (IO* io) {
	if (io == NULL)
		return -1;

	fclose(io->pipes_log_stream);
	fclose(io->events_log_stream);
	return 0;
}

// the func is used to init all descriptors of pipes.
// check if it is "useful" to pass arg
int create_pipes(IO* io) {
	size_t total_proc_count = io->proc_number + 1;

	io->channels = calloc(1, total_proc_count * total_proc_count * sizeof(ChannelHandle));
	for (int i = 0; i < total_proc_count; i++) {
		for (int j = 0; j < total_proc_count; j++) {
			if (i == j) {
				io->channels[i * total_proc_count + j].fd_read = -1;
				io->channels[i * total_proc_count + j].fd_write = -1;
				continue;
			}

			int fd[2];
			if (pipe(fd) < 0) {
				perror("create concrete pipe");
				return -1;
			} else {
				io->channels[i * total_proc_count + j].fd_read = fd[0];
				io->channels[i * total_proc_count + j].fd_write = fd[1];
				fprintf(io->pipes_log_stream,
					"from %d to %d (fd's id): read=%d write=%d\n",
					i, j,
					io->channels[i * total_proc_count + j].fd_read,
					io->channels[i * total_proc_count + j].fd_write);
			}
		}
	}
	return 0;
}


// parse -p option and return additional proc amount
int get_proc_num (int argc, char* argv[]) {
	int proc_num = -1;
	switch (getopt(argc, argv, "p:")) {
		case 'p': {
			proc_num = strtol(optarg, NULL, 10);
			if (optarg == NULL || proc_num < 0 || proc_num > MAX_PROCESS_ID)
				proc_num = 0;
			break;
		}
	}
	return proc_num;
}


void close_non_related_fd(IO* io, local_id id) {
	size_t total_proc_count = io->proc_number + 1;
	for (local_id i = 0; i < total_proc_count; i++) {
		for (local_id j = 0; j < total_proc_count; j++) {
			if (i == j)
				continue;
			if (i == id) {
				// from this process we can only write to other
				close(io->channels[i*total_proc_count +j].fd_read);
				fprintf(io->pipes_log_stream, "proc id=%d closed R[%d-%d]\n",
					id, i, j);
			}
			if (j == id) {
				// from this view our process could be only as destination
				close(io->channels[i*total_proc_count+j].fd_write);
				fprintf(io->pipes_log_stream, "proc id=%d closed W[%d-%d]\n",
					id, i, j);
			}
			if (j != id && i != id) {
				close(io->channels[i*total_proc_count+j].fd_write);
				close(io->channels[i*total_proc_count+j].fd_read);
				fprintf(io->pipes_log_stream, "proc id=%d closed RW[%d-%d]\n",
					id, i, j);
			}
		}
	}
	fprintf(io->pipes_log_stream,
		"====| PROC %d ENDED UP CLOSE UNRELATED PIPES!\n", id);
}


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
