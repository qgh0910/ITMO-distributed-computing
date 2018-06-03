#define _GNU_SOURCE  // used for getopt func
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // library for fcntl function
#include <getopt.h>

#include "io.h"
#include "util.h"
#include "banking.h"

// the func is used to init all descriptors of pipes.
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

int get_options (int argc, char* argv[], int *is_mutexl) {
	if (argc < 2)
		return -1;

	*is_mutexl = 0;
	struct option long_options[] = {
		{"mutexl", no_argument, (int*)is_mutexl, 1},
		{0, 0, 0, 0}
	};

	int option = -1;
	int proc_number = -1;
	
	while ((option = getopt_long (argc, argv, "p:", long_options, NULL))!= -1) {
		switch (option) {
			case -1:
				return -1;
			case 0:
				break;
			case 'p':
				proc_number = strtoul(optarg, NULL, 10);
				if (proc_number <= 0 || proc_number > MAX_PROCESS_ID)
					return -1;
				break;
			default:
				break;
		}
	}

	return proc_number;	
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
