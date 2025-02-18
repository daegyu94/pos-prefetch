#pragma once 

struct config_t {
    std::string addr;
    std::string port;
    int extent_cache;
    int extent_aligned_dispatch;
    int trace_fadvise;
};

extern struct config_t config;
