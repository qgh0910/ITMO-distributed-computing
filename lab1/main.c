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

int main(int argc, char* argv[]) {

	// TODO: parse -p option (see SRS)

	// TODO: open log files for logging

	if (create_pipes() < 0)
	    return -1;

	// TODO: fork all processes in loop with checking if forked successfully.
	// If child - then do all child's stuff.

	    // TODO: wait all other processes

	    // TODO: close all descriptors (e.g. log files)
}

// the func is used to init all descriptors of pipes.
// check if it is "useful" to pass arg


int create_pipes() {

}

int open_logs() {

}

int close_logs() {

}

int close_descriptors() {

}
