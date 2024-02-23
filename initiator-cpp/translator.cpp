#include <ctime>

#include "translator.h"
#include "metadata.h"
#include "extent.h"
#include "fiemap.h"
#include "btree.h"

#define DMFP_TRANS_INFO
#ifdef DMFP_TRANS_INFO
#define dmfp_trans_info(str, ...) \
    printf("[INFO] Translator::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_trans_info(str, ...) do {} while (0)
#endif

//#define DMFP_TRANS_DEBUG
#ifdef DMFP_TRANS_DEBUG
#define dmfp_trans_debug(str, ...) \
    printf("[DEBUG] Translator::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_trans_debug(str, ...) do {} while (0)
#endif


#define MAX_EXTENT_CACHE_SIZE (1 * 1024 * 1024)
#define BTREE_DEGREE 3

Translator::Translator(GRPCHandler &grpc_handler) : 
    _grpc_handler(grpc_handler), _lru_cache(MAX_EXTENT_CACHE_SIZE) { }

Translator::~Translator() { }

LRUCache& Translator::GetLRUCache() {
    return _lru_cache;
}

ExtentTree *Translator::BuildTree(DevIdInoPair &pair) {
    auto file_path = findFilePathFast(pair);
    if (file_path.empty()) {
        return nullptr;
    }
    
    std::vector<Extent *> ext_vec;
    int ret = getExtents(file_path, ext_vec);
    if (ret) {
        return nullptr;
    }
    
    ExtentTree *extent_tree = btree_new(sizeof(Extent *), 8, extent_compare, NULL);
    
    for (const auto ext : ext_vec) {
        dmfp_trans_debug("lba=%lu, pba=%lu, length=%u\n", ext.lba, ext.pba, ext.length);
        btree_set(extent_tree, &ext);
    }

    return extent_tree;
}

ExtentTree *Translator::CreateOrGetExtentTree(DevIdInoPair &pair, uint64_t file_size) {
    auto key = pair;
    auto value = _lru_cache.Get(key);
    ExtentTree *extent_tree = (ExtentTree *) value.second;

#ifdef DMFP_EXTENT_MONITORING
    std::lock_guard<std::mutex> lock_guard(_mtx); 
#endif

    if (extent_tree == nullptr) {
        extent_tree = BuildTree(pair);
        if (extent_tree) {
            auto key = pair;
            auto value = std::make_pair(file_size, extent_tree);

            auto ev_key = _lru_cache.Put(key, value);
            if (ev_key.first != -1) {
                filepath_map.Delete(ev_key);
            }
        }
    } else {
        if (value.first != file_size) {
            auto key = pair;
            auto ret = _lru_cache.Delete(key);
            
            ExtentTree *ret_extent_tree = (ExtentTree *) ret.second;
            if (ret_extent_tree != nullptr) {
                btree_free(ret_extent_tree);
            }

            extent_tree = BuildTree(pair);
            if (extent_tree) {
                auto ev_key = _lru_cache.Put(pair, 
                        std::make_pair(file_size, extent_tree));
                if (ev_key.first != -1) {
                    filepath_map.Delete(ev_key);
                }
            }
        } else {
            extent_tree = (ExtentTree *) value.second;
        }
    }

    return extent_tree;
}

#define MAX_BUFFER_MSGS 16
bool steering_on = false;
bool evaluating_readahead = false;
bool evaluating_reference = false;
bool evaluating_extent = evaluating_readahead || evaluating_reference;

void Translator::ProcessPageDeletion(BPFEvent *event) {
    PageDeletionEvent ev = { event->dev_id, event->ino, event->index, 
        event->file_size };
    dmfp_trans_debug("dev_id=%u, ino=%lu, index=%lu\n", 
            ev.dev_id, ev.ino, ev.index);
    
    _event_vec.push_back(ev);
    if (_event_vec.size() < MAX_BUFFER_MSGS) {
        return;
    }

    if (steering_on) {
        _event_vec = steering.Process(_event_vec);    
    }
    
    for (auto &ev : _event_vec) {
        DevIdInoPair pair = std::make_pair(ev.dev_id, ev.ino); 
        ExtentTree *extent_tree = 
            CreateOrGetExtentTree(pair, ev.file_size);
        
        if (extent_tree) {
            Extent ext(ev.index << PAGE_SHIFT);
            Extent *ret_ext = (Extent *) *(uint64_t *) btree_get(extent_tree, &ext);  
            if (!ret_ext) {
                continue; 
            }
            
            ret_ext->ClearRefCnt(ev.index);
            //ret_ext->Show();

            uint32_t subsys_id, ns_id;
            std::tie(subsys_id, ns_id) = mntpnt_map.Get2(ev.dev_id);
            if (steering_on && evaluating_extent) {
                /* (start_index, num_pages)*/
                std::vector<std::pair<uint64_t, uint32_t>> vec = 
                    evaluateExtent(ext, ev.index, evaluating_readahead, evaluating_reference);
                
                /* range may be splited */
                for (auto &v : vec) {
                    uint64_t pba = ret_ext->pba + (v.first << PAGE_SHIFT);
                    uint32_t length = v.second << PAGE_SHIFT; 

                    RpcMessage msg = { subsys_id, ns_id, pba, length };
                    _grpc_handler.Process(msg);
                } 
            } else {
                uint64_t pba = ret_ext->pba + (ev.index << PAGE_SHIFT);
                uint32_t length = -1; 

                RpcMessage msg = { subsys_id, ns_id, pba, length };
                _grpc_handler.Process(msg);
            }
        } else {
            // cannot found file info
        }
    }
    _event_vec.clear();
}

void Translator::ProcessPageReference(BPFEvent *event) {
    DevIdInoPair pair = std::make_pair(event->dev_id, event->ino); 
    ExtentTree *extent_tree = CreateOrGetExtentTree(pair, event->file_size);
    if (!extent_tree) {
        return;
    }
    uint64_t index = event->index;
    Extent ext(index << PAGE_SHIFT);
    Extent *ret_ext = (Extent *) *(uint64_t *) btree_get(extent_tree, &ext); 
    if (!ret_ext) {
        return;
    }
    ret_ext->AddRefCnt(index);
    //ret_ext->Show();
}

void Translator::ProcessReadpages(BPFEvent *event) {
    DevIdInoPair pair = std::make_pair(event->dev_id, event->ino); 
    ExtentTree *extent_tree = CreateOrGetExtentTree(pair, event->file_size);
    if (!extent_tree) {
        return;
    }
    uint64_t index = event->index;
    Extent *ret_ext = nullptr;

    for (uint16_t i = 0; i < event->readahead_size; i++) {
        uint64_t next_index = index + i; 
        if (!ret_ext || !ret_ext->IsKeyInRange(next_index << PAGE_SHIFT)) {
            Extent ext(next_index << PAGE_SHIFT);
            ret_ext = (Extent *) *(uint64_t *) btree_get(extent_tree, &ext);      
        }
        
        if (!ret_ext) {
            printf("[ERROR] cannot find extent\n");
        }

        ret_ext->ClearRefCnt(next_index);
        ret_ext->SetReadaheadBitmap(next_index);
        //ret_ext->Show();
    } 
}

void Translator::ProcessUnlink(BPFEvent *event) {
    LRUCacheKeyType key = std::make_pair(event->dev_id, event->ino);
#ifdef DMFP_EXTENT_MONITORING
    std::lock_guard<std::mutex> lock_guard(_mtx); 
#endif
    ExtentTree *ret_extent_tree = (ExtentTree *) _lru_cache.Delete(key).second;
    if (ret_extent_tree) {
        btree_free(ret_extent_tree);
    }
}


void Translator::GetExtentTrees(std::vector<LRUCacheValueType> &vec)  {
#ifdef DMFP_EXTENT_MONITORING
    std::lock_guard<std::mutex> lock_guard(_mtx); 
#endif
    _lru_cache.Gets(vec);
}