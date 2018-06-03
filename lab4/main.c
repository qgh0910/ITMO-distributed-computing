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

// implement lamport time functions
static timestamp_t process_time = 0;

timestamp_t get_lamport_time() { return process_time; }
void increase_time() {	process_time += 1; }
void set_actual_time(timestamp_t new_time) { process_time = new_time > process_time ? new_time : process_time; }

int main(int argc, char *argv[])
{
	IO io;
	int is_mutexl;
	int proc_number;

	// parse options
	proc_number = get_options (argc, argv, &is_mutexl);
	if (proc_number == -1) return 2;

	io = (IO) {
		.proc_id = PARENT_ID, 
		.proc_number = proc_number,
		.is_mutexl = is_mutexl
	};

	// open logs, create pipes
	if (open_log_streams (&io) < 0) {
		perror("unsuccessful open logs");
		return -2;
	}
	if (create_pipes(&io) < 0) {
		perror("unsuccessful create pipes");
		return -1;
	}

	// fork children, start children processes
	for (int i = 1; i <= io.proc_number; i++) {		
		fflush(io.events_log_stream);
		fflush(io.pipes_log_stream);	

        pid_t pid = fork();
        if (0 > pid) {
            exit(EXIT_FAILURE);
        } else if (0 == pid) {            
            int ret = child_process(&io, i);
            exit(ret);
        }
    }

	// do parent process, close logs, pipes
    parent_process(&io);

	return 0;
}
