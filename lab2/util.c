#define _GNU_SOURCE  // used for getopt func
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // library for fcntl function

#include "io.h"
#include "util.h"
#include "banking.h"

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
				// error checking for fcntl
				int res = fcntl(fd[0], F_SETFL, O_NONBLOCK);
				int res2 = fcntl(fd[1], F_SETFL, O_NONBLOCK);
    			if (res < 0 || res2 < 0) {
					perror("O_NONBLOCK");
					return -2;
				}
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


// Returns number of child processes and init_balances in args by pointer
int get_options (int argc, char* argv[], balance_t* balances) {
	int proc_num = -1;
	switch (getopt(argc, argv, "p:")) {
		case 'p': {
			proc_num = strtol(optarg, NULL, 10);
			if (optarg == NULL || proc_num < 0 || proc_num > MAX_PROCESS_ID)
				proc_num = 0;
			break;
		}
	}
	if (argc == 2) {
		fprintf(stderr, "Incorrect number of options!\n");
		exit(-1);
	}
	if (argc != proc_num + 3) {
		fprintf(stderr, "Incorrect number of options! Expected %d, has %d.\n",
				proc_num + 3, argc);
		exit(-2);
	}
	int start = 3;
	// printf("proc num %d\n", proc_num);
	for (int i = 0; i < proc_num; i++) {
		char* str = argv[i + start];
		// printf("option #%d = %s\n", i + start, str);
		balances[i + 1] = strtol(str, NULL, 10);
		if (balances[i+1] < 0 || str == NULL) {
			fprintf(stderr, "Incorrect balance option! see %d param.\n",
				    i+start);
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


// get required channel from channels table to perform data transmition from src to dest
ChannelHandle* get_channel_handle (IO* io, local_id src_id, local_id dest_id) {
	if (io == NULL)
		return NULL;
	int total_proc_count = io->proc_number+1;
	if (src_id < 0 || dest_id < 0 || src_id > total_proc_count-1 || dest_id > total_proc_count-1) {
		return NULL;
	} else {
		return &io->channels [src_id * total_proc_count + dest_id];
	}
}

Message get_empty_ACK() {
	Message msg = (Message) {
		.s_header = (MessageHeader) {
			.s_magic = MESSAGE_MAGIC,
			.s_payload_len = 0,
			.s_type = ACK,
			.s_local_time = get_physical_time()
		},
		.s_payload = {(char)NULL}
	};
	return msg;
}

Message get_empty_STOP() {
	Message msg = (Message) {
		.s_header = (MessageHeader) {
			.s_magic = MESSAGE_MAGIC,
			.s_payload_len = 0,
			.s_type = STOP,
			.s_local_time = get_physical_time()
		},
		.s_payload = {(char)NULL}
	};
	return msg;
}

void _print_history_(BalanceHistory* bh, int proc) {
	int len = bh->s_history_len;
	for (int i = 0; i < len; i++) {
		BalanceState bs = bh->s_history[i];
		printf("(proc %d) time %d : $%d\n", bh->s_id, bs.s_time, bs.s_balance);
	}
}
