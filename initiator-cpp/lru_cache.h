#pragma once

#include <list>
#include <unordered_map>
#include <iostream>
#include <vector>

#include "my_types.h"

using LRUCacheKeyType = DevIdInoPair;
using LRUCacheValueType = std::pair<uint64_t, void *>;

class LRUCache {
private:
    using CacheListIterator = std::list<std::pair<LRUCacheKeyType, 
          LRUCacheValueType>>::iterator;
    std::unordered_map<LRUCacheKeyType, CacheListIterator> _map;
    std::list<std::pair<LRUCacheKeyType, LRUCacheValueType>> _cache;
    size_t _capacity; 

public:
    LRUCache(size_t);
    ~LRUCache();

    LRUCacheKeyType Put(const LRUCacheKeyType &, const LRUCacheValueType &);
    LRUCacheValueType Get(const LRUCacheKeyType &);
    LRUCacheValueType Delete(const LRUCacheKeyType &);
    
    void Gets(std::vector<LRUCacheValueType> &);

    void Display();
    size_t GetCapacity();
    size_t GetSize();
};
