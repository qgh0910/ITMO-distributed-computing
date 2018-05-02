#include <unistd.h>
#include <string.h>

#include "ipc.h"
#include "io.h"

ChannelHandle* get_channel_handle (IO* io, local_id src_id, local_id dest_id);


int send(void * self, local_id dst, const Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	// TODO: check if dest == src. Is it an error?

	ChannelHandle channel = get_channel_handle (io, io->proc_id, dst);
	if (channel == NULL)
		return -2;

	size_t full_msg_size = sizeof (msg->s_header + msg->s_header.s_payload_len);
	int write_result = write (channel->fd_write, msg, full_msg_size);
	if (write_result != full_msg_size)
		return -3;
	else
		return 0;
}

int send_multicast(void * self, const Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	int send_result = 0;
	for (local_id i = 0; i < io->proc_number; i++) {
		send_result = send (self, i, msg);
		if (send_result < 0) {
			return -1;
		} 
	}

	return 0;
}

int receive(void * self, local_id from, Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	// TODO: check if from == to. Is it an error?
	ChannelHandle channel = get_channel_handle(io, from, io->proc_id);
	if (channel == NULL)
		return -2;

	char* received_buff = new char[MAX_MESSAGE_LEN];
	int read_result = 0;
	do {
		read_result = read (channel->fd_read, received_buff, MAX_MESSAGE_LEN);
		if (read_result > 0) {							// message received => break from loop;
			memcpy (msg, received_buff, read_result);
			break;
		}	
		usleep(10000);		// TODO: check if value '10000' is suitable
	} while (1);

	return 0;
}

int receive_any(void * self, Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	char* received_buff = new char[MAX_MESSAGE_LEN];
	int read_result = 0;
	ChannelHandle channel = NULL;

	do {
		for (local_id i = 0; i < io->proc_number; i++) {
			if (i == io->proc_id) {
				continue;
			}
			channel = get_channel_handle(io, from, io->proc_id);
			if (channel == NULL)
				return -2;	
			read_result = read (channel->fd_read, received_buff, MAX_MESSAGE_LEN);
			if (read_result > 0) {							// message received => break from loop;
				memcpy (msg, received_buff, read_result);
				return 0;
			}
		}
		usleep(10000);

	} while (1);
}


// get required channel from channels table to perform data transmition from src to dest
ChannelHandle* get_channel_handle (IO* io, local_id src_id, local_id dest_id) {
	if (io == NULL)
		return NULL;
	int proc_number = io->proc_number;
	if (src_id < 0 || dest_id < 0 || src_id > proc_number || dest_id > proc_number) {
		return NULL;
	} else {
		return io->channels [src_id * proc_number + dest_id];
	}
}
