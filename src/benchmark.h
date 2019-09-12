// benchmark.h

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <sys/time.h>
// #include <sys/resource.h>
#include <stdio.h>
#include <string.h>

double get_time(void);
void benchmark_start(void);
void benchmark_elapsed(void);

void benchmark_slot_resetall(void);
void benchmark_slot_start(int slot);
void benchmark_slot_update(int slot);
void benchmark_slot_print(int slot);
void benchmark_slot_printall(void);

#endif
