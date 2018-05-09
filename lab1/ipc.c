#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "ipc.h"
#include "io.h"

ChannelHandle* get_channel_handle (IO* io, local_id src_id, local_id dest_id);


int send(void * self, local_id dst, const Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	// TODO: check if dest == src. Is it an error?
	// if (dst == io->proc_id) {
	// 	return 0;
	// }

	ChannelHandle *channel = get_channel_handle (io, io->proc_id, dst);
	if (channel == NULL) {
		printf("NULL! %d %d\n", io->proc_id, dst);
		return -2;
	}
	// printf("write id : %d read_id: %d\n", channel->fd_write, channel->fd_read);

	size_t full_msg_size = sizeof msg->s_header + msg->s_header.s_payload_len;
	int write_result = write (channel->fd_write, msg, full_msg_size);
	printf("Hello from send! %d res:%d\n" , io->proc_id, write_result);
	if (write_result != full_msg_size) {
		fprintf(io->pipes_log_stream, "proc id=%d can't sent to %d! Failed!\n",
			io->proc_id, dst);
		// printf("Ok\n");
		return -3;
	}
	else {
		if (fprintf(io->pipes_log_stream, "proc id=%d sent to %d successfully!\n",
			io->proc_id, dst) < 0)
			printf("error!\n");
		// printf("Ok!\n");
		return 0;
	}

}

int send_multicast(void * self, const Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	int send_result = 0;
	for (local_id i = 0; i <= io->proc_number; i++) {
		printf("Hello from send_multicast! %d\n" , io->proc_id);
		send_result = send (self, i, msg);
		// if (send_result < 0) {
		// 	return -1;
		// }
	}

	return 0;
}

int receive(void * self, local_id from, Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	// TODO: check if from == to. Is it an error?
	ChannelHandle *channel = get_channel_handle(io, from, io->proc_id);
	if (channel == NULL)
		return -2;

	char received_buff[MAX_MESSAGE_LEN];
	int read_result = 0;
	do {
		read_result = read (channel->fd_read, received_buff, MAX_MESSAGE_LEN);
		if (read_result > 0) {							// message received => break from loop;
			memcpy (msg, received_buff, read_result);
			break;
		}
		usleep(10000);		// TODO: check if value '10000' is suitable
	} while (1);

	fprintf(io->pipes_log_stream, "proc id=%d received msg from %d !\n",
		io->proc_id, from);
	return 0;
}

int receive_any(void * self, Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	char* received_buff [MAX_MESSAGE_LEN];
	int read_result = 0;
	ChannelHandle *channel = NULL;

	do {
		for (local_id i = 0; i < io->proc_number; i++) {
			if (i == io->proc_id) {
				continue;
			}
			channel = get_channel_handle(io, i, io->proc_id);
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
	int total_proc_count = io->proc_number+1;
	if (src_id < 0 || dest_id < 0 || src_id > total_proc_count-1 || dest_id > total_proc_count-1) {
		return NULL;
	} else {
		return &io->channels [src_id * total_proc_count + dest_id];
	}
}
