import time 
import os
import inspect

from my_types import *

file_path = "/proc/dmfp/evicted_pages"

procfs_debug_on = False
def procfs_debug(*args):
    if procfs_debug_on:
        func_name = inspect.currentframe().f_back.f_code.co_name
        print("[DEBUG] procfs: {}: {}".format(func_name, " ".join(map(str, args))))

def run_procfs_monitor(event_queue):
    if not os.path.exists(file_path):
        print('[WARNING] File not found: {}, stop thread' .format(file_path))
        return

    with open(file_path, 'r') as file:
        while True:
            output = []

            try:
                for i in range(lines_per_chunk):
                    output.append(file.readline())

            except Exception as e:
                print('[ERROR] {}' .format(e))
                break

            for item in output:
                if item == '':
                    continue

                values = item.strip().split(',')
                cli_id, ino, index = map(int, values)
               
                """ 
                TODO: save metadata info of dev_id, file_size in cleancache module 
                or just hardcoded
                """ 
                dev_id = -1
                file_size = -1
                page_info = PageInfo(EventType.EVENT_CLEANCACHE_REPL, 
                        dev_id, ino, index, file_size)
                
                procfs_debug(page_info)

                event_queue.put(page_info, block=False)

            time.sleep(0.1)

