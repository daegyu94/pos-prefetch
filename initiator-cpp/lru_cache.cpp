#include "lru_cache.h"

#include "stat.h"

LRUCache::LRUCache(size_t size) {
    _capacity = size;
}

LRUCache::~LRUCache() { }

LRUCacheKeyType LRUCache::Put(const LRUCacheKeyType &key, const LRUCacheValueType &value) {
    LRUCacheKeyType victim = { -1, -1UL };

    if (_map.find(key) == _map.end()) {
        if (_cache.size() == _capacity) {
            victim = _cache.back().first;
            _cache.pop_back();
            _map.erase(victim);
            counter.lru_cache_evict++;
        }
        counter.lru_cache_miss++;
    } else {
        _cache.erase(_map[key]);
        counter.lru_cache_hit++;
    }

    // update reference
    _cache.push_front(std::make_pair(key, value));
    _map[key] = _cache.begin();
    
    counter.lru_cache_size = _cache.size();
    return victim;
}

LRUCacheValueType LRUCache::Get(const LRUCacheKeyType &key) {
    LRUCacheValueType ret = { -1UL, nullptr };

    if (_map.find(key) != _map.end()) {
        ret = _map[key]->second;
        _cache.erase(_map[key]);
        _cache.push_front(std::make_pair(key, ret));
        _map[key] = _cache.begin();
        counter.lru_cache_hit++;
    }

    return ret;
}

LRUCacheValueType LRUCache::Delete(const LRUCacheKeyType &key) {
    LRUCacheValueType ret = { -1UL, nullptr };

    if (_map.find(key) != _map.end()) {
        ret = _map[key]->second;
        _cache.erase(_map[key]);
        _map.erase(key);
        counter.lru_cache_delete++;
    }

    counter.lru_cache_size = _cache.size();
    return ret;
}

void LRUCache::Gets(std::vector<LRUCacheValueType> &vec) {
    for (auto it = _cache.begin(); it != _cache.end(); it++) {
        /* (ino, exten_tree_ptr) */
        LRUCacheValueType v = std::make_pair(std::get<1>(it->first),
                std::get<1>(it->second));
        vec.push_back(v);
    }
}

void LRUCache::Display() {
    // Iterate in the deque and print
    // all the elements in it
    for (auto it = _cache.begin(); it != _cache.end(); it++) {
        std::cout << "(" << std::get<0>(it->first) << ", " << 
            std::get<1>(it->first) << ") ";
    }
    std::cout << std::endl;
}

size_t LRUCache::GetCapacity() {
    return _capacity;
}

size_t LRUCache::GetSize() {
    return _cache.size();
}
