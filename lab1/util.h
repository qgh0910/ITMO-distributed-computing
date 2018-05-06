
#include "io.h"

int log_event (char* message);
int log_pipe (char* message);

int do_child_work ();

int create_pipes    (IO* io);
int get_proc_num    (int argc, char* argv[]);
void close_non_related_fd(IO* io, local_id id);
int child_process(IO* io, local_id proc_id);
int synchronize_with_others(
	uint16_t payload_len,
	MessageType type,
	timestamp_t local_time,
	char* payload,
	IO* proc
);
