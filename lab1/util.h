
#include "io.h"

int log_event (char* message);
int log_pipe (char* message);

int do_child (IO io);
int do_parent (IO io);

int create_pipes    (IO* io);
int get_proc_num    (int argc, char* argv[]);
