#ifndef __IO_H__
#define __IO_H__

#include <sys/types.h>
#include <stdio.h>
#include "ipc.h"
#include "proc_queue.h"

// describes read/write channel
typedef struct
{
	int fd_write;
	int fd_read;
} ChannelHandle;

// io structure contains list of existing channels
typedef struct
{
	local_id proc_id;  				// this process id
	size_t proc_number;  			// amount of other processes connected with this proc
	ChannelHandle* channels;

	FILE* events_log_stream;
	FILE* pipes_log_stream;

	local_id working_proc_number;	
	int is_mutexl;					// is critical section enabled
	
	Node* proc_queue;				// process lamport queue for critical section
} IO;

#endif
