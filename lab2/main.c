#define _GNU_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "io.h"
#include "util.h"
#include "logging.h"
#include "process.h"
#include "banking.h"

void get_balance_history_from_all(IO* io, AllHistory* all_history);

void wait_messages_from_all(IO* io, MessageType type) {
	for (size_t i = 1; i <= io->proc_number; i++) {
		Message msg = {{0}};
		while(1) {
			int res = receive((void*)io, i, &msg);
			if (res != 0)
				continue;
			if (msg.s_header.s_type == type)
				break;
		}
	}
}

// return codes:
//	0		success
// 	-1		create_pipes failed
int main(int argc, char* argv[]) {
	// init base structure, where all pipes stored
	balance_t init_balances[MAX_PROCESS_ID + 1] = {0};
	int i = get_options(argc, argv, (balance_t*)init_balances);
	i = i >= 0 ? i : 0;
	IO io = (IO) {
		.proc_id = PARENT_ID,
		.proc_number = i
	};

	open_log_streams (&io);
	// printf("done open log streams!\n");
	if (create_pipes(&io) < 0) {
		perror("create pipes fucked up");
		return -1;
	}

	for (size_t i = 1; i <= io.proc_number; i++) {
		// needs to avoid repetition
		fflush(io.events_log_stream);
		fflush(io.pipes_log_stream);

		pid_t pid = fork();
		if (pid < 0)
			exit(EXIT_FAILURE);
		else if (pid == 0) {  // if a child
			// as process(!!!) starts we need to close all
			// non-related file descriptors
			close_non_related_fd(&io, (local_id)i);
			// printf("init_balances[%zu] = %d\n", i, init_balances[i]);
			int child_ret = child_process(&io, (local_id)i, init_balances[i]);
			exit(child_ret);  // end up child process
		}
	}
	// sync START
	close_non_related_fd(&io, (local_id)PARENT_ID);
	wait_messages_from_all(&io, STARTED);
	// printf("%s\n", "all started - said main");
	fprintf(io.events_log_stream, log_received_all_started_fmt,
            get_physical_time(), PARENT_ID);

	// do WORK
	bank_robbery((void*)&io, io.proc_number);
	// printf("%s\n", "bank robbery done");

	// sync by sending STOP and waiting DONE
	Message stop_msg = get_empty_STOP();
	send_multicast((void*)&io, (const Message*)&stop_msg);
	wait_messages_from_all(&io, DONE);
	fprintf(io.events_log_stream, log_received_all_done_fmt,
            get_physical_time(), PARENT_ID);

	// get HISTORY and print it
	AllHistory all_history = (AllHistory) {
		.s_history_len = io.proc_number,
		.s_history = {{0}}
	};
	get_balance_history_from_all(&io, &all_history);
	print_history((const AllHistory*)&all_history);

	while(wait(NULL) > 0);  // waiting end of childs (see POSIX)
	close_log_streams(&io);
}

void _print_history_(BalanceHistory* bh, int proc) {
	printf("=======================PROC %d=======================\n", proc);
	int len = bh->s_history_len;
	for (int i = 0; i < len; i++) {
		BalanceState bs = bh->s_history[i];
		printf("(proc %d) time %d : $%d\n", bh->s_id, bs.s_time, bs.s_balance);
	}
}

void get_balance_history_from_all(IO* io, AllHistory* all_history) {
	for (size_t i = 1; i <= io->proc_number; i++) {
		Message msg = {{0}};
		while(1) {
			int res = receive((void*)io, i, &msg);
			if (res != 0) {
				usleep(1000);
				continue;
			}
			if (msg.s_header.s_type != BALANCE_HISTORY) {
				char* str = "Parent expected BALANCE_HISTORY(5), got %d";
				fprintf(io->pipes_log_stream, str, msg.s_header.s_type);
				usleep(1000);
				continue;
			}
			memcpy(&all_history->s_history[i], &msg.s_payload,
				msg.s_header.s_payload_len);
			// get last balance
			BalanceHistory bh = all_history->s_history[i];
			_print_history_(&bh, i);
			BalanceState bs = bh.s_history[bh.s_history_len-1];
			balance_t balance = bs.s_balance;

			fprintf(io->events_log_stream, log_done_fmt,
				get_physical_time(), (int)i, balance);
			break;
		}
	}
}
