#pragma once

#include <stdint.h>

//#define LATENCY_BREAKDOWN

struct StatCounter {
    uint64_t grpc_num_send_msgs;
    
    uint64_t trans_pba;
    uint64_t trans_pba_failed;

    uint64_t lru_cache_hit;
    uint64_t lru_cache_miss;
    uint64_t lru_cache_delete;
    uint64_t lru_cache_evict;
    uint64_t lru_cache_size;

    uint64_t ext_found;
    uint64_t ext_not_found;

    uint64_t fiemap_succ;
    uint64_t fiemap_ioctl_failed;
    uint64_t fiemap_file_not_found_failed;

    uint64_t event_queue_size;
    
    uint64_t event_open_abs;
    uint64_t event_open_abs_long;
    uint64_t event_open_rel;
    
    uint64_t event_page_deletion;
    uint64_t event_page_referenced;
    uint64_t event_vfs_unlink;
    uint64_t event_readpages;
    uint64_t event_cleancache_repl;
    
    uint64_t bpf_lost_open;
    uint64_t bpf_lost_page_access;
    uint64_t bpf_lost_readpages;
    
    uint64_t extent_cache;
    uint64_t request_alignment;
};

extern StatCounter counter;

static inline uint64_t GetTotalProcessedEvents() {
    return counter.event_page_deletion + \
        counter.event_page_referenced + \
        counter.event_vfs_unlink + \
        counter.event_readpages + \
        counter.event_cleancache_repl;
}


enum {
    BR_EH_ENQ, 
    BR_EH_DEQ, 
    BR_REQ_ALIGN, 
    BR_EXT_CACHE, 
    BR_RPC, 

    BR_MAX,
};

struct LatencyBreakdown {
    uint64_t elapseds[BR_MAX];
};

extern struct LatencyBreakdown br;
extern const char *br_names[];

static inline uint64_t elapsed_us(int name)
{
    return br.elapseds[name] / 1000;
}

static inline double elapsed_avg_us(int name, uint64_t cnt)
{
    if (cnt == 0) {
        return 0.0;
    } else{
        return (double) br.elapseds[name] / 1000 / cnt;
    }
}

#ifdef LATENCY_BREAKDOWN

#define _(x)                    br_time_##x
#define br_declare_ts(x)        struct timespec _(x) = {0, 0}
#define br_start_ts(x)          clock_gettime(CLOCK_MONOTONIC, &_(x))
#define br_end_ts(x, name)      do {                                \
    struct timespec end = {0, 0};                                   \
    clock_gettime(CLOCK_MONOTONIC, &end);                           \
    br.elapseds[name] +=								            \
    (end.tv_sec - _(x).tv_sec) * (size_t) 1e9 +                     \
    (end.tv_nsec - _(x).tv_nsec);                                   \
} while (0)
#define br_end_ts_with_lat(x, name, lat)      do {                  \
    struct timespec end = {0, 0};                                   \
    clock_gettime(CLOCK_MONOTONIC, &end);                           \
    br.elapseds[name] += lat +								        \
    (end.tv_sec - _(x).tv_sec) * (size_t) 1e9 +                     \
    (end.tv_nsec - _(x).tv_nsec);                                   \
} while (0)

#define br_add_lat(name, lat)      do {                             \
    br.elapseds[name] += lat;								        \
} while (0)

#else

#define br_declare_ts(x)              do {} while (0)
#define br_start_ts(x)                do {} while (0)
#define br_end_ts(name, x)            do {} while (0)
#define br_end_ts_with_lat(name, x)   do {} while (0)
#define br_add_lat(name, lat)         do {} while (0)

#endif
