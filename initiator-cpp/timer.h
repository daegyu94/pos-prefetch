#pragma once 
#include <time.h>

extern uint64_t br_elapseds[];

enum {
    BR_ALLOC,
    BR_FREE,

    MAX_BR
};

#define _(x)                    br_time_##x
#define br_declare_ts(x)       struct timespec _(x) = {0, 0}
#define br_start_ts(x)         clock_gettime(CLOCK_MONOTONIC, &_(x))
#define br_end_ts(name, x)     do {                         \
    struct timespec end = {0, 0};                           \
    clock_gettime(CLOCK_MONOTONIC, &end);                   \
    br_elapseds[name] +=									\
    (end.tv_sec - _(x).tv_sec) * (size_t) 1e9 +             \
    (end.tv_nsec - _(x).tv_nsec);                           \
} while (0)
