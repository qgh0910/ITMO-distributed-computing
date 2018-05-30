#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "ipc.h"
#include "io.h"
#include "util.h"

int send(void * self, local_id dst, const Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	ChannelHandle *channel = get_channel_handle (io, io->proc_id, dst);
	if (channel == NULL) {
		return -2;
	}

	size_t full_msg_size = sizeof msg->s_header + msg->s_header.s_payload_len;
	int write_result = write (channel->fd_write, msg, full_msg_size);
	if (write_result != full_msg_size) {
		fprintf(io->pipes_log_stream, "proc id=%d can't sent to %d, type: %d! Failed!\n",
			io->proc_id, dst, msg->s_header.s_type);
		// printf("proc id=%d can't sent to %d, type: %d! Failed!\n",
			// io->proc_id, dst, msg->s_header.s_type);
		return -3;
	} else {
		fprintf(io->pipes_log_stream, "proc id=%d sent to %d, type: %d successfully!\n",
			io->proc_id, dst, msg->s_header.s_type);
		// printf("proc id=%d sent to %d, type: %d successfully!\n",
			// io->proc_id, dst, msg->s_header.s_type);
		return 0;
	}
}

int send_multicast(void * self, const Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	int send_result = 0;
	for (local_id i = 0; i <= io->proc_number; i++) {
		send_result = send (self, i, msg);
	}

	return 0;
}

int receive(void * self, local_id from, Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	ChannelHandle *channel = get_channel_handle(io, from, io->proc_id);
	if (channel == NULL)
		return -2;

	// init read buffer
	char received_buff[MAX_MESSAGE_LEN];
	memset(received_buff, '\0', MAX_MESSAGE_LEN);
	int header_read_len = read(	// read header of msg to retrieve payload size
		channel->fd_read,
		received_buff,
		sizeof(MessageHeader)
	);
	if ( header_read_len <= 0)
		return -3;
	MessageHeader* msg_header = (MessageHeader*) received_buff;
	size_t payload_len = msg_header->s_payload_len;
	// printf("receive(%d) from %d \n--bytes of header: %d\n--type: %d\n",
	// 	io->proc_id, from, header_read_len, msg_header->s_type);
	int paload_read_len = read(				// read payload of msg
		channel->fd_read,
		received_buff + header_read_len,  	// we write to single buf
		payload_len							// size of payload
	);
	if (paload_read_len >= 0)
		memcpy (msg, received_buff, paload_read_len + header_read_len);

	fprintf(io->pipes_log_stream, "received from %d in %d with type %d\n",
		from, io->proc_id, msg_header->s_type);
	return 0;
}

// returns process id from which message came from
int receive_any(void * self, Message * msg) {
	if ((self == NULL) || (msg == NULL))
		return -1;
	IO *io = (IO*)self;

	for (local_id i = 0; i <= io->proc_number; i++) {
		int ret = receive(self, i, msg);
		// printf("--receive_any (%d) ret code = %d\n", io->proc_id, ret);
		if (ret == 0)
			return i;
	}
	return -2;
}
