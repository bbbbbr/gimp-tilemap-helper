// benchmark.h

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>

double get_time();
void benchmark_start();
void benchmark_elapsed();

void benchmark_slot_resetall();
void benchmark_slot_start(int slot);
void benchmark_slot_update(int slot);
void benchmark_slot_print(int slot);
void benchmark_slot_printall();