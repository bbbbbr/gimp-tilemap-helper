// benchmark.c

#include "benchmark.h"

static double last_time;

#define max_slots 10

static double slot_accum[max_slots];
static double slot_last[max_slots];

double get_time(void)
{
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t.tv_sec + t.tv_usec*1e-6;
}

void benchmark_start(void) {
    last_time = get_time();
}

void benchmark_elapsed(void) {
    printf(" ==> Elapsed: %.4f\n", get_time() - last_time);
    last_time = get_time();
}


void benchmark_slot_resetall(void) {
    int c;
    for (c = 0; c < max_slots; c++) {
        slot_accum[c] = 0;
    }
}


void benchmark_slot_start(int slot) {
    if (slot < max_slots)
        slot_last[slot] = get_time();
}


void benchmark_slot_update(int slot) {
    if (slot < max_slots)
        slot_accum[slot] += get_time() - slot_last[slot];
}


void benchmark_slot_print(int slot) {
    if (slot < max_slots)
        printf(" ==> (%d) Elapsed: %.4f\n", slot, slot_accum[slot]);
}


void benchmark_slot_printall(void) {
    int c;
    for (c = 0; c < max_slots; c++) {
        if (slot_accum[c] != 0)
            printf(" ==> (s: %d) Elapsed: %.4f\n", c, slot_accum[c]);
    }
}
