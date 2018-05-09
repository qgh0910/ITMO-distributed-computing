#include "io.h"

int get_proc_num(int argc, char* argv[]);
int create_pipes(IO* io);
void close_non_related_fd(IO* io, local_id id);
