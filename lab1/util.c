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

int open_log_streams () {
	return 0;
}

int log_event (char* message) {
	return 0;
}

int log_pipe (char* message) {
	return 0;
}

// the func is used to init all descriptors of pipes.
// check if it is "useful" to pass arg
int create_pipes(IO* io) {
	size_t proc_num = io->proc_number;

	io->channels = calloc(1, proc_num * proc_num * sizeof(ChannelHandle));
	for (int i = 0; i <= proc_num; i++) {
		for (int j = 0; j <= proc_num; j++) {
			if (i == j) {
				io->channels[i * proc_num + j].fd_read = -1;
				io->channels[i * proc_num + j].fd_write = -1;
				continue;
			}

			int fd[2];
			if (pipe(fd) < 0) {
				perror("create concrete pipe");
				return -1;
			} else {
				io->channels[i * proc_num + j].fd_read = fd[0];
				io->channels[i * proc_num + j].fd_write = fd[1];
				printf("from %d to %d: read=%d\t write=%d", i, j, io->channels[i * proc_num + j].fd_read, io->channels[i * proc_num + j].fd_write);
			}
		}
		printf("\n");
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
	size_t num = io->proc_number;
	for (local_id i = 0; i <= num; i++) {
		for (local_id j = 0; j <= num; j++) {
			if (i == j)
				continue;
			if (i == id) {
				// from this process we can only write to other
				close(io->channels[i*num +j].fd_read);
				// TODO: log
			}
			if (j == id) {
				// from this view our process could be only as destination
				close(io->channels[i*num+j].fd_write);
				// TODO: log
			}
			if (!(j == id || i == id)) {
				close(io->channels[i*num+j].fd_write);
				close(io->channels[i*num+j].fd_read);
				// TODO: log
			}
		}
	}
}


// all child process work including sync and useful work
int child_process(IO* io, local_id proc_id) {
	// set IO struct for the process
	IO this_process = {
		.proc_id = proc_id,
		.proc_number = io->proc_number,
		.channels = io->channels
	};

	// set for messages and sync
	char payload[MAX_PAYLOAD_LEN];
	int payload_len = sprintf(payload, log_started_fmt, proc_id, getpid(), getppid());
	MessageType type = STARTED;
	timestamp_t t = 0;

	synchronize_with_others(payload_len, type, t, payload, &this_process);
	do_child_work();

	// set for msg and sync
	payload_len = sprintf(payload, log_done_fmt, proc_id);
	type = DONE;

	synchronize_with_others(payload_len, type, t, payload, &this_process);
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
