#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>

#include <unistd.h>

#include "monitor.h"
#include "stat.h"

Monitor::Monitor(EventQueue &event_queue, Translator &translator) : 
    _event_queue(event_queue), _translator(translator) {

    _stat_thd = std::thread(&Monitor::RunStat, this);

    _procfs_thd = std::thread(&Monitor::RunProcfs, this);
}

Monitor::~Monitor() {
    if (_stat_thd.joinable()) {
        _stat_thd.join();
    }
    if (_procfs_thd.joinable()) {
        _procfs_thd.join();
    }
}

void Monitor::WriteCounter(void) {                        
    std::ofstream outfile("log.counter");
	if (!outfile.is_open()) {
        std::cerr << "Error: Unable to open file for writing." << "\n";
        return;
    }
    
    outfile << "grpc_num_send_msgs: " << counter.grpc_num_send_msgs 
        << ",\ntrans_pba: " << counter.trans_pba         
        << ", trans_pba_failed: " << counter.trans_pba_failed       
        << ",\nlru_cache_hit: " << counter.lru_cache_hit             
        << ", lru_cache_miss: " << counter.lru_cache_miss           
        << ", lru_cache_delete: " << counter.lru_cache_delete 
        << ", lru_cache_evict: " << counter.lru_cache_evict 
        << ", lru_cache_size: " << counter.lru_cache_size         
        << ",\next_found: " << counter.ext_found 
        << ", ext_not_found: " << counter.ext_not_found             
        << ",\nfiemap_succ: " << counter.fiemap_succ                 
        << ", fiemap_ioctl_failed: " << counter.fiemap_ioctl_failed 
        << ", fiemap_file_not_found_failed: " << counter.fiemap_file_not_found_failed
        << ",\nevent_queue_size: " << counter.event_queue_size       
        << ", event_open_abs: " << counter.event_open_abs           
        << ", event_open_abs_long: " << counter.event_open_abs_long 
        << ", event_open_rel: " << counter.event_open_rel           
        << ", event_page_deletion: " << counter.event_page_deletion 
        << ", event_page_referenced: " << counter.event_page_referenced
        << ", event_vfs_unlink: " << counter.event_vfs_unlink       
        << ", event_readpages: " << counter.event_readpages         
        << ",\nbpf_lost_open: " << counter.bpf_lost_open             
        << ", bpf_lost_page_access: " << counter.bpf_lost_page_access
        << ", bpf_lost_readpages: " << counter.bpf_lost_readpages   
        << "\n";

    outfile.close();                                                    
}

void Monitor::WriteExtent(void) {
    std::ofstream outfile("log.extent");
    if (!outfile.is_open()) {
        std::cerr << "Error: Unable to open file for writing." << "\n";
        return;
    }

    std::vector<LRUCacheValueType> extent_tree_vec;
    _translator.GetExtentTrees(extent_tree_vec);

    for (auto &x : extent_tree_vec) {
        uint64_t ino = x.first;
        ExtentTree *extent_tree = (ExtentTree *) x.second;
        std::vector<Extent *> extent_vec;

        btree_ascend(extent_tree, NULL, extent_iter, &extent_vec);
        int i = 0;
        for (auto &extent : extent_vec) {
            extent->Show(ino, i, outfile);
            i++;
        }
    }

    outfile.close();
}

void Monitor::RunStat() {
    while (true) {
        WriteCounter();
#ifdef DMFP_EXTENT_MONITORING
        WriteExtent();
#endif
        sleep(2);
    }
}

void Monitor::ReadProcfs(void) {
    std::vector<std::string> output;
    const int lines_per_chunk = 1024;
    const int num_info = 3;

    std::ifstream infile("/proc/dmfp/evicted_pages");
    if (!infile.is_open()) {
        std::cerr << "[WARNING] Failed to open procfs file" << std::endl;
        return;
    }

    for (int i = 0; i < lines_per_chunk; ++i) {
        std::string line;
        std::getline(infile, line);
        output.push_back(line);
    }

    for (const auto &item : output) {
        if (item.empty()) {
            continue;
        }

        std::istringstream iss(item);
        std::vector<uint64_t> values;
        uint64_t value;

        while (iss >> value) {
            values.push_back(value);
            if (iss.peek() == ',') {
                iss.ignore();
            }
        }

        if (values.size() != num_info) {
            std::cerr << "[ERROR] Invalid data format." << std::endl;
            continue;
        }

        int cli_id = values[0];
        uint64_t ino = values[1];
        uint64_t index = values[2];
        /* TODO: cleancache should pass this info by procfs */
        uint32_t dev_id = -1;
        uint64_t file_size = -1;

        BPFEvent *event = new BPFEvent(EVENT_CLEANCACHE_REPL, dev_id, ino, index, file_size);
        _event_queue.Enqueue((void *) event);
    }

    infile.close();
}

void Monitor::RunProcfs() {
    while (true) {
        ReadProcfs();
        usleep(500 * 1000);
    }
}
