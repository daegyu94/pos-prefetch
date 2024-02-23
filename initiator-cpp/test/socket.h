#pragma once

#include <thread>

#include "event_queue.h"

class SocketClient {
public:
    SocketClient(const char* socket_path, EventQueue &event_queue);
    ~SocketClient();

    bool Connect();
    void Start();
    void Stop();

private:
    int server_socket;
    const char* socket_path;
    bool is_running;
    std::thread thd;
    
    EventQueue &event_queue;

    void ReceiveEvent();
};
