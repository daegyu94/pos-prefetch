//#include "prefetch_dispatcher.h"
#include "prefetch_worker.h"

//#include "src/include/memory.h"

#include <unistd.h>

namespace pos {
PrefetchWorker::PrefetchWorker() {
    thread_ = new std::thread(&PrefetchWorker::Run, this);
    dispatcher_ = new PrefetchDispatcher();
}

PrefetchWorker::~PrefetchWorker() {
    delete dispatcher_;

    thread_->join();
    delete thread_;
}

void PrefetchWorker::Work(void) {
    PrefetchInitMeta initMeta;
    PrefetchTgtMeta tgtMeta;

    if (traceOn) {
        if (!traceHandler_->OpenTrace()) {
            return;
        }
        while (traceHandler_->ReadLine(initMeta)) {
            tgtMeta = PrefetchTgtMeta(initMeta);
            void *ptr = pos::Memory<BLOCK_SIZE>::Alloc(1);
            std::pair<void *, size_t> dest = std::make_pair(ptr, BLOCK_SIZE);

            PrefetchIOConfig *ioConfig = new PrefetchIOConfig(tgtMeta, dest);
#if 1
            RBAStateManager *rbaStateManager =
                RBAStateServiceSingleton::Instance()->GetRBAStateManager(tgtMeta.arrayId);
            bool ownershipAcquired =
                rbaStateManager->BulkAcquireOwnership(tgtMeta.volumeId,
                        ChangeByteToBlock(tgtMeta.rba), 1);

            if (!ownershipAcquired) {
                delete ioConfig;
                continue;
            }
#endif
            dispatcher_->Execute(ioConfig);
        }
        traceHandler_->CloseTrace();
    } else {
        /* TODO: get input from handler */
        tgtMeta = PrefetchTgtMeta(initMeta);
        /* delete at completion */
        PrefetchIOConfig *ioConfig = new PrefetchIOConfig(tgtMeta, dest);
        dispatcher_->Execute(ioConfig);
    }
}

void PrefetchWorker::Run(void) {
    while (true) {
        if (traceOn) {
            bool on = traceHandler_->CheckEnable();
            if (!on) {
                usleep(10 * 1000);
                continue;
            }
        }

        Work();
    }
}
} // namespace pos
