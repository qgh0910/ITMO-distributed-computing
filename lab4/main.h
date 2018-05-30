#ifndef _MAIN_H_
#define _MAIN_H_

#include "pa2345.h"

// lamport time functions for PA4
timestamp_t get_lamport_time();
void set_actual_time(timestamp_t new_time);
void increase_time();

#endif
