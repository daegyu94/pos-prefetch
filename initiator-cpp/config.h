#pragma once 

struct config_t {
    std::string addr;
    std::string port;
    int extent_cache;
    int extent_aligned_dispatch;
};

extern struct config_t config;
