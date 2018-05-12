#define _GNU_SOURCE  // used for getopt func
#include <stdlib.h>
#include <unistd.h>

#include "io.h"
#include "util.h"

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
				// perror("create concrete pipe");
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
