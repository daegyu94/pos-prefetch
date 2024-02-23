#pragma once

#include <stdint.h>
#include <thread>

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
};

extern StatCounter counter;
