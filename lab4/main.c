#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "util.h"
#include "main.h"
#include "logging.h"
#include "process.h"

static timestamp_t process_time = 0;

// implement lamport time functions
timestamp_t get_lamport_time() {
	return process_time;
}
void set_actual_time(timestamp_t new_time) {
	process_time = new_time > process_time ? new_time : process_time;
}
void increase_time() {
	process_time += 1;
}

void wait_messages_from_all(IO* io, MessageType type) {
	for (size_t i = 1; i <= io->proc_number; i++) {
		Message msg = {{0}};
		while(1) {
			int res = receive((void*)io, i, &msg);
			if (res != 0)
				continue;			
			if (msg.s_header.s_type == type)
				break;
		}
		set_actual_time(msg.s_header.s_local_time);
		increase_time();
	}
}

int main(int argc, char* argv[])
{
	IO io;
	int is_mutexl;
	int proc_number;

	proc_number = get_options (argc, argv, &is_mutexl);
	if (proc_number == -1) return 2;

	io = (IO) {
		.proc_id = PARENT_ID, 
		.proc_number = proc_number,
		.is_mutexl = is_mutexl
	};
	open_log_streams (&io);
	if (create_pipes(&io) < 0) {
		perror("create pipes fucked up");
		return -1;
	}
	for (size_t i = 1; i <= io.proc_number; i++) {
		// needs to avoid repetition
		fflush(io.events_log_stream);
		fflush(io.pipes_log_stream);

		pid_t pid = fork();
		if (pid < 0)
			exit(EXIT_FAILURE);
		else if (pid == 0) {  // if a child
			// as process(!!!) starts we need to close all
			// non-related file descriptors
			close_non_related_fd(&io, (local_id)i);
			// printf("init_balances[%zu] = %d\n", i, init_balances[i]);
			int child_ret = child_process(&io, (local_id)i);
			exit(child_ret);  // end up child process
		}
	}

	// sync START
	close_non_related_fd(&io, (local_id)PARENT_ID);	
	Message msg = get_empty_msg();

	wait_messages_from_all(&io, STARTED);

	// count completed children	
	while (io.completed_proc_counter < io.proc_number) {
		if (receive_any(&io, &msg) < 0) return 1;
		set_actual_time(msg.s_header.s_local_time);
		increase_time();

		if( msg.s_header.s_type == DONE ) io.completed_proc_counter++;
	}

	// wait for processes end
	while(wait(NULL) > 0);  
	close_log_streams(&io);

	return 0;
}
