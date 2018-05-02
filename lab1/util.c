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
	
}

int log_event (char* message) {

} 

int log_pipe (char* message) {

}