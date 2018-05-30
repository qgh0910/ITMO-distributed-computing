#include "io.h"
#include "ipc.h"
#include "banking.h"

int get_options (int argc, char* argv[], int *is_mutexl);
int create_pipes(IO* io);
void close_non_related_fd(IO* io, local_id id);

ChannelHandle* get_channel_handle (IO* io, local_id src_id, local_id dest_id);
size_t get_msg_total_size_from_header ();

Message get_empty_STOP();
Message get_empty_ACK();
Message get_empty_msg();
void _print_history_(BalanceHistory* bh, int proc);
