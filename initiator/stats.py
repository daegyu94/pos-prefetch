import logging
import schedule
import time

from lock import *
#logging.basicConfig(filename='counter.log', level=logging.INFO, filemode='w')

class Stats:
    # grpc
    send_msgs = 0
    qsize = 0
    trans_pba_fast = 0
    trans_pba_failed = 0
    trans_pba_extent_filtered = 0

    # extent cache
    ext_cache_hit = 0    
    ext_cache_miss = 0    
    ext_cache_delete = 0    
    ext_cache_evict = 0    
    ext_cache_size = 0
            
    ext_found = 0
    ext_not_found = 0
    
    fiemap_succ = 0
    fiemap_ioctl_failed = 0
    fiemap_not_found_failed = 0

    # bpf
    open_abs = 0
    open_abs_long = 0
    open_rel = 0
    lost_open = 0
    page_deletion = 0
    lost_page_deletion = 0
    lost_queue_full_page_deletion = 0 

    page_referenced = 0
    lost_queue_full_page_referenced = 0
    
    ext4_mpage_readpages = 0
    lost_ext4_mpage_readpages = 0
    lost_queue_full_ext4_mpage_readpages = 0
    
    vfs_unlink = 0
    lost_queue_full_vfs_unlink = 0

def log_stats():
    with open('counter.log', 'w') as file:
        file.write(f"send_msgs={Stats.send_msgs}, qsize={Stats.qsize}, trans_pba: fast={Stats.trans_pba_fast}, failed={Stats.trans_pba_failed}, filtered={Stats.trans_pba_extent_filtered}\n"
                f"ext_cache: hit={Stats.ext_cache_hit}, miss={Stats.ext_cache_miss}, delete={Stats.ext_cache_delete}, evict={Stats.ext_cache_evict}, size={Stats.ext_cache_size}\n"
                f"ext found={Stats.ext_found}, not_found={Stats.ext_not_found}\n"
                f"fiemap succ={Stats.fiemap_succ}, failed ioctl={Stats.fiemap_ioctl_failed}, not_found={Stats.fiemap_not_found_failed}\n"
                f"open abs={Stats.open_abs}, abs_long={Stats.open_abs_long}, rel={Stats.open_rel}, lost={Stats.lost_open}\n"
                f"page_deletion={Stats.page_deletion}, lost={Stats.lost_page_deletion}, lost_queue_full={Stats.lost_queue_full_page_deletion}\n"
                f"page_referenced={Stats.page_referenced}, lost_queue_full={Stats.lost_queue_full_page_referenced}\n"
                f"ext4_mpage_readpages={Stats.ext4_mpage_readpages}, lost={Stats.lost_ext4_mpage_readpages} lost_queue_full={Stats.lost_queue_full_ext4_mpage_readpages}\n"
                f"vfs_unlink={Stats.vfs_unlink}, lost_queue_full={Stats.lost_queue_full_vfs_unlink}\n")

def log_extent_info(translator):
    with open('extent.log', 'w') as file:
        kv_list = translator.get_lru_cache().get_all_kv()
        
        file.write("num_extent_tree={}\n" .format(len(kv_list))) 
        for kv in kv_list:
            ino = kv[0][1]
            extent_tree = kv[1][1]
            item_list = list(extent_tree.inorder())
            
            for item in item_list:
                extent = item.v
                file.write("ino={}, lba={}, length={}, ref_cnt=(avg={:.3f}, max={})\n" 
                        .format(ino, extent.lba, extent.length, 
                            extent.get_avg_ref_cnt(), extent.get_max_ref_cnt()))

def run_stats_monitor(translator):
    while True:
        log_stats()
        if lock_enable:
            log_extent_info(translator)
        time.sleep(2)
