#include "io.h"

int get_proc_num(int argc, char* argv[]);
int create_pipes(IO* io);
void close_non_related_fd(IO* io, local_id id);
ChannelHandle* get_channel_handle (IO* io, local_id src_id, local_id dest_id);
