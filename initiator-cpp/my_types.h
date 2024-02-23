#pragma once

#include <cstdint>

using DevIdInoPair = std::pair<uint32_t, uint64_t>;

namespace std {
    template <>
        struct hash<DevIdInoPair> {
            size_t operator()(const DevIdInoPair& k) const {
                return hash<uint32_t>()(k.first) ^ hash<uint64_t>()(k.second);
            }
        };
}

struct PageDeletionEvent {
    uint32_t dev_id;
    uint64_t ino;
    uint64_t index;
    uint64_t file_size;
};

struct BPFEvent {
    uint16_t type;
    uint32_t dev_id;
    uint64_t ino;
    uint64_t index;
    uint64_t file_size;
    uint32_t readahead_bitmap;
    uint16_t readahead_size;

    BPFEvent() {}

    BPFEvent(uint16_t type, uint32_t dev_id, uint64_t ino, uint64_t index, 
            uint64_t file_size, 
            uint32_t readahead_bitmap = 0, uint16_t readahead_size = 0) {
        this->type = type;
        this->dev_id = dev_id;
        this->ino = ino;
        this->index = index;
        this->file_size = file_size;
        this->readahead_bitmap = readahead_bitmap;
        this->readahead_size = readahead_size;
    } 
};

struct RpcMessage {
    uint32_t subsys_id;
    uint32_t ns_id;
    uint64_t pba;
    uint32_t length;
};

enum event_type {
    EVENT_OPEN_ENTRY = 0,
    EVENT_OPEN_END,
    EVENT_CLOSE,
    EVENT_PAGE_DELETION,
    EVENT_PAGE_REFERENCED,
    EVENT_EXT4_MPAGE_READPAGES,
    EVENT_VFS_UNLINK,
    EVENT_CLEANCACHE_REPL,
};

#define PAGE_SHIFT 12
#define PAGE_SIZE 4096
