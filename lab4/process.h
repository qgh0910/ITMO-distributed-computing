#include "io.h"
#include "banking.h"
#include "ipc.h"

int child_process(IO* io, local_id proc_id);
int synchronize_with_others(
	uint16_t payload_len,
	MessageType type,
	timestamp_t local_time,
	char* payload,
	IO* proc
);

// waiting and process TRANSFER and STOP messages
int do_child_work(IO* io);
// update empty gaps in timeline history
int fill_empty_history_entries(BalanceHistory* history, timestamp_t cur_time);
// func INCREASES balance on delta (+=) and update history
int update_balance_history(BalanceHistory* balance_history,
	balance_t delta);

int send_history_to_parent(IO* io, BalanceHistory* history);
int do_transfer_from_parent(BalanceHistory* balance_history,
	TransferOrder* transfer, IO* io, Message msg);
int do_transfer_from_child(BalanceHistory* balance_history,
	TransferOrder* transfer, IO* io);
