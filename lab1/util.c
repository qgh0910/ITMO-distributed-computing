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
