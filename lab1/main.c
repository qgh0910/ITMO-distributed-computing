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

int main(int argc, char* argv[]) {
	// init base structure, where all pipes stored
	IO io = {0};
	io.proc_id = 0;
	io.proc_number = 0;

	int i = get_proc_num(argc, argv);
	i = i >= 0 ? i : 0;
	io.proc_number = i;

	// TODO: open log files for logging

	if (create_pipes(&io) < 0) {
		perror("create pipes fucked up");
		return -1;
	}

	// TODO: fork all processes in loop with checking if forked successfully.
	// If child - then do all child's stuff.

	    // TODO: wait all other processes

			// TODO: close all descriptors (e.g. log files)
}
