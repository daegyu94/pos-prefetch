#pragma once 

#include <queue>
#include <mutex>
#include <condition_variable>

/*
#define MAX_EVENTLIST_SIZE 32
struct EventInfo {
    int event_type;
    uint64_t ino;
};

struct EventList {
    struct EventInfo event[MAX_EVENTLIST_SIZE];
    int size;
};
*/
class EventQueue {
public:
    void Enqueue(void *item) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        queue.push(item);
        condition.notify_one();
    }

    void *Dequeue() {
        std::unique_lock<std::mutex> lock(queue_mutex);

        while (queue.empty() && is_running) {
            condition.wait(lock);
        }

        if (!queue.empty()) {
            void *result = queue.front();
            queue.pop();
            return result;
        }

        return nullptr;
    }

    void Stop() {
        is_running = false;
        condition.notify_all();
    }
    
    size_t Size() {
        return queue.size();
    }

private:
    std::queue<void *> queue;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool is_running = true;
};
