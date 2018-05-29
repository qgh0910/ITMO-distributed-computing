#ifndef _MAIN_H_
#define _MAIN_H_

timestamp_t get_lamport_time();
void increase_time();
void set_actual_time(timestamp_t new_time);

#endif