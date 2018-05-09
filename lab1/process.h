#include "io.h"

int child_process(IO* io, local_id proc_id);
int synchronize_with_others(
	uint16_t payload_len,
	MessageType type,
	timestamp_t local_time,
	char* payload,
	IO* proc
);
int do_child_work();
