#pragma once

#include "translator.h"
#include "event_queue.h"

#define DMFP_EXTENT_MONITORING 

class Monitor {
public:
    Monitor(EventQueue &, Translator &);
    ~Monitor();

private:
    std::thread _stat_thd;
    std::thread _procfs_thd;
    
    EventQueue &_event_queue;
    Translator &_translator;

    void WriteCounter(void);
    void WriteExtent(void);
    void RunStat(void);

    void ReadProcfs(void);
    void RunProcfs(void);
};
