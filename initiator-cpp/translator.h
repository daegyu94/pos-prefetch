#pragma once

#include <cstdint>
#include <list>
#include <queue>
#include <unordered_map>
#include <iostream>
#include <cstring>

#include "btree.h"
#include "fiemap.h"
#include "lru_cache.h"
#include "steering.h"
#include "grpc_handler.h"
#include "my_types.h"

class Translator {
	public:
	    Translator(GRPCHandler &);
		~Translator();

        LRUCache& GetLRUCache();
        
        std::string FindFilePathFast(DevIdInoPair &);
		ExtentTree *BuildTree(DevIdInoPair &);
		ExtentTree *CreateOrGetExtentTree(DevIdInoPair &, uint64_t file_size);

        void ProcessPageDeletion(BPFEvent *event);
        void ProcessPageReference(BPFEvent *event);
        void ProcessReadpages(BPFEvent *event);
        void ProcessUnlink(BPFEvent *event);
		    
        void GetExtentTrees(std::vector<LRUCacheValueType> &);
        
    private:
		LRUCache _lru_cache;
        std::vector<PageDeletionEvent> _event_vec;
        GRPCHandler &_grpc_handler;

        std::mutex _mtx; /* for extent stat monitoring */
};
