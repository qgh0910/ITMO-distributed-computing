#include "io.h"
#include "common.h"

int open_log_streams (IO* io) {
	if (io == NULL)
		return -1;

	io->events_log_stream = fopen(events_log, "a+");
	if (io->events_log_stream == NULL)
		return -2;

	io->pipes_log_stream = fopen(pipes_log, "a+");
	if (io->pipes_log_stream == NULL)
		return -3;

	return 0;
}

int close_log_streams (IO* io) {
	if (io == NULL)
		return -1;

	fclose(io->pipes_log_stream);
	fclose(io->events_log_stream);
	return 0;
}
