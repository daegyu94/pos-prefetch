#include <iostream>
#include <vector>

#include "event_queue.h"

void Producer(EventQueue& queue) {
    for (int i = 0; i < 10; ++i) {
        EventList* eventList = new EventList;
        eventList->size = i + 1;

        for (int j = 0; j < eventList->size; ++j) {
            eventList->event[j].event_type = i * 10 + j;
            eventList->event[j].ino = i * 100 + j;
        }

        queue.Enqueue(eventList);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    queue.Stop();
}

void Consumer(EventQueue& queue) {
    void* item;
    while ((item = queue.Dequeue()) != nullptr) {
        EventList* eventList = static_cast<EventList*>(item);

        std::cout << "Consumed EventList (size=" << eventList->size << "):" << std::endl;
        for (int i = 0; i < eventList->size; ++i) {
            std::cout << "  Event " << i << ": type=" << eventList->event[i].event_type
                      << ", ino=" << eventList->event[i].ino << std::endl;
        }

        delete eventList;
    }
}

int main() {
    EventQueue event_queue;

    std::thread producer_thread(Producer, std::ref(event_queue));
    std::thread consumer_thread(Consumer, std::ref(event_queue));

    producer_thread.join();
    consumer_thread.join();

    return 0;
}
