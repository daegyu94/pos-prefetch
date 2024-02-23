#pragma once

#include <thread>

#include "event_queue.h"
#include "translator.h"

class EventHandler {
public:
    EventHandler(EventQueue &, Translator &);
    ~EventHandler();
    
    void Start();
    void Wait();
    void Stop();

private:
    bool is_running;
    std::thread thd;

    EventQueue &_event_queue;
    Translator &_translator;

    void ProcessEvent(void* item);
    void HandleEvents(void);
};
