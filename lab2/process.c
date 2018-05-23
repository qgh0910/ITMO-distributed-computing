#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>

#include "io.h"
#include "pa2345.h"
#include "process.h"
#include "banking.h"
#include "util.h"

// all child process work including sync and useful work
int child_process(IO* io, local_id proc_id, balance_t init_balance) {
	// set IO struct for the process
	IO this_process = {
		.proc_id = proc_id,
		.proc_number = io->proc_number,
		.channels = io->channels,
		.events_log_stream = io->events_log_stream,
		.pipes_log_stream = io->pipes_log_stream
	};

	// init BalanceHistory
	BalanceHistory balance_history = (BalanceHistory) {
		.s_id = proc_id,
		.s_history_len = 1,
		.s_history = {{0}}
	};
	timestamp_t time = get_physical_time();
	for (timestamp_t i = 0; i <= time; i++) {
        balance_history.s_history[i] = (BalanceState) {
            .s_balance = init_balance,
            .s_time = i,
            .s_balance_pending_in = 0
        };
    }
    balance_history.s_history_len = time + 1;

	// set for messages and sync
	char payload[MAX_PAYLOAD_LEN];
	int payload_len = sprintf(payload, log_started_fmt, get_physical_time(),
							  proc_id, getpid(), getppid(),
							  balance_history.s_history[0].s_balance);
	MessageType type = STARTED;
	timestamp_t t = 0; // used only for synchronization

	// sync STARTED
	fputs(payload, this_process.events_log_stream);
	synchronize_with_others(payload_len, type, t, payload, &this_process);
	fprintf(this_process.events_log_stream, log_received_all_started_fmt,
			get_physical_time(), proc_id);

	// DO WORK
	do_child_work(&this_process, &balance_history);
	printf("done child work\n");

	// DONE
	uint8_t last_index = balance_history.s_history_len - 1;
	payload_len = sprintf(payload, log_done_fmt, get_physical_time(), proc_id,
		balance_history.s_history[last_index].s_balance);
	fputs(payload, this_process.events_log_stream);
	// fclose(this_process.events_log_stream);
	// fclose(this_process.pipes_log_stream);
	type = DONE;

	// sync DONE
	synchronize_with_others(payload_len, type, t, payload, &this_process);
	fprintf(this_process.events_log_stream, log_received_all_done_fmt,
		get_physical_time(), proc_id);

	// send HISTORY to parent
	timestamp_t cur_time = get_physical_time();
	fill_empty_history_entries(&balance_history, cur_time);
	child_before_exit(&this_process, &balance_history);
	return 0;
}


int child_before_exit(IO* io, BalanceHistory* history) {
	Message msg;
	msg.s_header = (MessageHeader) {
		.s_magic = MESSAGE_MAGIC,
		.s_payload_len = sizeof *history - (MAX_T + 1 - history->s_history_len) * sizeof *history->s_history,
		.s_type = BALANCE_HISTORY,
		.s_local_time = get_physical_time()
	};
	memcpy(msg.s_payload, history, msg.s_header.s_payload_len);
	int res = send(io, PARENT_ID, &msg);
	return res;
}


int synchronize_with_others(
	uint16_t payload_len,
	MessageType type,
	timestamp_t local_time,
	char* payload,
	IO* proc)
{
	// fullfill message
	Message msg;
	msg.s_header = (MessageHeader) {
		.s_magic = MESSAGE_MAGIC,
		.s_payload_len = payload_len,
		.s_type = type,
		.s_local_time = local_time
	};
	strncpy(msg.s_payload, payload, payload_len);

	send_multicast((void*)proc, (const Message *)&msg);

	// wait for messages from others
	// it's kinda "stub" which will prevent you to move on
	for (size_t i = 1; i <= proc->proc_number; i++)
		if (i == proc->proc_id)
			continue;
		else
			while (receive((void*)proc, i, &msg) != 0);
	return 0;
}

// waiting and process TRANSFER and STOP messages
int do_child_work(IO* io, BalanceHistory* balance_history)
{
	printf("doing work (%d)...\n", io->proc_id);
	while(1) {
		Message msg = {{0}};
		local_id src = receive_any((void*)io, &msg);
		// printf("--do_child_work(%d): received from %hhd with type: %d \n",
			// io->proc_id, src, msg.s_header.s_type);
        if (src < 0) {
			usleep(1000);
			continue;
		}
		switch (msg.s_header.s_type) {
			case TRANSFER: {
				TransferOrder* transfer = (TransferOrder*) msg.s_payload;
				if (src == PARENT_ID) {	//  this == transfer->src => decrease bal
					update_balance_and_history(balance_history,
						transfer->s_amount * (-1));
					send((void*)io, (local_id)transfer->s_dst, (const Message *)&msg);
					fprintf(io->events_log_stream, log_transfer_out_fmt,
                			get_physical_time(), io->proc_id,
                			transfer->s_amount, transfer->s_dst);
					// printf("--do_child_work() : after update log\n");
				}
				else { //  this == transfer->dst => increase balance
					fprintf(io->events_log_stream, log_transfer_in_fmt,
						get_physical_time(), io->proc_id,
						transfer->s_amount, transfer->s_dst);
					update_balance_and_history(balance_history,
						transfer->s_amount);
					Message empty_ACK = get_empty_ACK();
					send((void*)io, PARENT_ID, (const Message *)&empty_ACK);
				}
				break;
			}
			case STOP: { // STOP --> end up receiving messages
				fprintf(io->pipes_log_stream, "proc %d received STOP msg.\n",
					io->proc_id);
				return 0;
			}
			default: {
				char* str = "proc %d received msg with type %d from proc %d.\n";
				fprintf(io->pipes_log_stream, str,
					io->proc_id, msg.s_header.s_type, src);
				break;
			}
		}
	}
	return 0;
}


// func INCREASES balance on delta (+=) and update history
int update_balance_and_history(BalanceHistory* balance_history, balance_t delta)
{
	timestamp_t cur_time = get_physical_time();
	fill_empty_history_entries(balance_history, cur_time);
	balance_history->s_history_len = cur_time + 1;  // update new boundary
	balance_t cur_balance = balance_history->s_history[cur_time-1].s_balance;
	balance_history->s_history[cur_time] = (BalanceState) {
		.s_balance =  cur_balance + delta,
		.s_time = cur_time,
		.s_balance_pending_in = 0
	};
	return 0;
}


// update empty gaps in timeline history
int fill_empty_history_entries(BalanceHistory* history, timestamp_t cur_time) {
	uint8_t last_index = history->s_history_len;
	BalanceState last_hist_entry = history->s_history[last_index - 1];
	for (timestamp_t i = last_index + 1; i < cur_time; i++) {
		history->s_history[i] = last_hist_entry;
		history->s_history[i].s_time = i;	// obligatory to update time!!!
	}
	return 0;
}
