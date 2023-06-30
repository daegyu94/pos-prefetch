import logging
import schedule
import time

#logging.basicConfig(filename='counter.log', level=logging.INFO, filemode='w')

class Stats:
    # grpc
    send_msgs = 0
    qsize = 0
    trans_pba_fast = 0
    trans_pba_slow = 0
    
    # extent cache
    ext_cache_hit = 0    
    ext_cache_miss = 0    
    ext_cache_inv = 0    
    ext_cache_evict = 0    
    
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
    
def log_stats():
    #logging.info(f"{Stats.send_msgs} {Stats.trans_pba_fast} {Stats.trans_pba_slow}")
    with open('counter.log', 'w') as file:
        file.write(f"send_msgs={Stats.send_msgs}, qsize={Stats.qsize}, trans_pba fast={Stats.trans_pba_fast}, slow={Stats.trans_pba_slow}\n"
                f"ext_cache hit={Stats.ext_cache_hit}, miss={Stats.ext_cache_miss}, inv={Stats.ext_cache_inv}, evict={Stats.ext_cache_evict}\n"
                f"ext found={Stats.ext_found}, not_found={Stats.ext_not_found}\n"
                f"fiemap succ={Stats.fiemap_succ}, failed ioctl={Stats.fiemap_ioctl_failed}, not_found={Stats.fiemap_not_found_failed}\n"
                f"open abs={Stats.open_abs}, abs_long={Stats.open_abs_long}, rel={Stats.open_rel}, lost={Stats.lost_open}\n"
                f"page_deletion={Stats.page_deletion}, lost={Stats.lost_page_deletion}\n")

# 5초마다 총 수행 횟수 로그 기록
schedule.every(5).seconds.do(log_stats)

def run_stats_monitor():
    while True:
        schedule.run_pending()
        time.sleep(1)
