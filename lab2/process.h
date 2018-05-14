#include "io.h"
#include "banking.h"
#include "ipc.h"

int child_process(IO* io, local_id proc_id, balance_t init_balance);
int synchronize_with_others(
	uint16_t payload_len,
	MessageType type,
	timestamp_t local_time,
	char* payload,
	IO* proc
);

// waiting and process TRANSFER and STOP messages
int do_child_work(IO* io, BalanceHistory* balance_history);
// update empty gaps in timeline history
int fill_empty_history_entries(BalanceHistory* history, timestamp_t cur_time);
// func INCREASES balance on delta (+=) and update history
int update_balance_and_history(BalanceHistory* balance_history,
	balance_t delta);
