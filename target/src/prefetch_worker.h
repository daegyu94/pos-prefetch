#pragma once

#include "singleton.h" // "src/lib/singleton.h"

//#include "prefetch_dispatcher.h"

#include <thread>

namespace pos {
class PrefetchWorker {
    public:
        PrefetchWorker();
        ~PrefetchWorker();

    private:
        void Work(void);
        void Run(void);

        /* TODO : create a thread pool and in charge of each initiator */
        std::thread* thread_;
        PrefetchDispatcher *dispatcher_;
};

using PrefetchWorkerSingleton = Singleton<PrefetchWorker>;
} // namespace pos
