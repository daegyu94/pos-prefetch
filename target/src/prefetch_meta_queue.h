#pragma once

#include <tbb/concurrent_queue.h>

namespace pos {
struct PrefetcMeta {
    std::string subsysnqn;
    uint32_t ns_id;
    uint64_t pba;

    PrefetchMeta(std::string s, uint32_t n, uint64_t p) {
        subsysnqn = s;
        ns_id = n;
        pba = p;
    }
};


using PrefetchMetaSmartPtr = std::shared_ptr<PrefetchMeta>;

/* 'T' should be a pointer type of certain object */
template<typename T>
class PrefetchMetaQueue
{
public:
    PrefetchMetaQueue(void) { }
    
    virtual ~PrefetchMetaQueue(void) { }

    virtual bool IsEmpty(void) const {
        return queue_.empty();
    }

    virtual void Enqueue(const T obj) {
        queue_.push(obj);
    }

    virtual T Dequeue(void) {
        T t;
        if (queue_.try_pop(t)) 
            return t;
        else 
            return nullptr;
    }

private:
    tbb::concurrent_queue<T> queue_;
};
} // namespace pos
