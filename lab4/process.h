#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "ipc.h"
#include "io.h"

int child_process(IO* io, local_id proc_id);
int child_work(IO* io);
int parent_process(IO* io);
int parent_work (IO* io);
int wait_all_messages (IO* io, MessageType type);
int wait_child_processes(IO* io);

#endif
