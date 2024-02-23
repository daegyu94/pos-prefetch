#pragma once

#include <thread>
#include <unordered_map>

#include <bcc/BPF.h>

#include "event_queue.h"

class BPFTracer {
public:
    BPFTracer();
    ~BPFTracer() {}
    
    void UpdateMntMap(uint32_t);
    void Wait();

    static EventQueue *_event_queue;
private:
    ebpf::BPF _bpf;
    std::thread _thd;
    static std::unordered_map<uint64_t, std::string> _entries;
    
    static void HandleOpenEvents(void *, void *, int);    
    static void HandlePageAccessEvents(void *, void *, int);    
    static void HandleReadpagesEvents(void *, void *, int); 

    static void HandleLostOpenEvents(void *, uint64_t);    
    static void HandleLostPageAccessEvents(void *, uint64_t);    
    static void HandleLostReadpagesEvents(void *, uint64_t); 

    void Run();
};
