#ifndef __IO_H__
#define __IO_H__

#include <sys/types.h>
#include "ipc.h"

// describes read/write channel; io structure contains list of existing channels 
typedef struct 
{
	int fd_write;
	int fd_read;	
} ChannelHandle;

typedef struct 
{
	local_id proc_id;
	size_t proc_number;
	ChannelType* channels;
} IO;