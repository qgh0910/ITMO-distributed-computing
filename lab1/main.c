#define _GNU_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include "io.h"
#include "util.h"

void wait_msg(IO* io) {
	Message msg = {{0}};
	for (size_t i = 1; i <= io->proc_number; i++) {
		while(receive((void*)io, i, &msg) != 0);
	}
}


void wait_others_messages(IO* io) {
	wait_msg(io);  // wait for STARTED
	fprintf(io->events_log_stream, log_received_all_started_fmt, PARENT_ID);
	wait_msg(io);  // wait for DONE
	fprintf(io->events_log_stream, log_received_all_done_fmt, PARENT_ID);
}


int main(int argc, char* argv[]) {
	// init base structure, where all pipes stored
	IO io = {.proc_id = PARENT_ID};

	int i = get_proc_num(argc, argv);
	i = i >= 0 ? i : 0;
	io.proc_number = i;

	open_log_streams (&io);

	if (create_pipes(&io) < 0) {
		perror("create pipes fucked up");
		return -1;
	}

	for (size_t i = 1; i <= io.proc_number; i++) {
		fflush(io.events_log_stream);
		fflush(io.pipes_log_stream);
		pid_t pid = fork();
		if (pid < 0)
			exit(EXIT_FAILURE);
		else if (pid == 0) {
			// as process(!!!) starts we need to close all
			// non-related file descriptors
			close_non_related_fd(&io, (local_id)i);
			int child_ret = child_process(&io, (local_id)i);
			exit(child_ret);  // end up child process
		}
	}
	close_non_related_fd(&io, (local_id)PARENT_ID);
	wait_others_messages(&io);
	printf("AFTER WAITING FROM PARENT(0)\n");
	while(wait(NULL) > 0);  // waiting end of childs (see POSIX)
	close_log_streams(&io);
}
