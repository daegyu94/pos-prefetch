#import threading
import queue 
import asyncio
from functools import lru_cache
import inspect

from metadata import *
from extent_tree import *
from ioctl import *
from lru_cache import *
from stats import *
from my_types import *
from steering import *


BTREE_DEGREE = 3 # number of nodes: t - 1 <= N <= 2t - 1
MAX_EXTENT_CACHE_SIZE = 1024

translator_debug_on = False
def translator_debug(*args):
    if translator_debug_on:
        func_name = inspect.currentframe().f_back.f_code.co_name
        print("[DEBUG] Translator: {}: {}".format(func_name, " ".join(map(str, args))))

class Translator():
    def __init__(self, meta_dict, fpath_dict, steering):
        self._queue = queue.Queue()
        self._meta_dict = meta_dict
        self._fpath_dict = fpath_dict
        self._steering_on = steering
        """
                                      [0]         [1]
        key: (dev_id, ino), value: (file_size, extent_tree)
        """
        self._lru_cache = LRUCache(MAX_EXTENT_CACHE_SIZE) 
    
    def __del__(self):
        self._queue.join()
    
    def get_lru_cache(self):
        return self._lru_cache

    def find_file_path_fast(self, dev_id, ino):
        ret = self._fpath_dict.get((dev_id, ino))
        if ret is None:
            return None

        path_type = ret[0]
        file_path = ret[1].decode('utf-8');

        if path_type == 2:
            mnt_path = self._meta_dict.get_mntpnt(dev_id) 
            return mnt_path + file_path
        else: 
            return file_path

    def build_tree(self, dev_id, ino):
        file_path = self.find_file_path_fast(dev_id, ino)
        if file_path is None:
            #print("file_path is None")
            return None
        
        #start_time = time.time_ns()
        extents = Fiemap(file_path).do()
        #end_time = time.time_ns()
        #elapsed_time_microseconds = (end_time - start_time) / 1000 
        #print(f"fiemap: {elapsed_time_microseconds:.2f} us")

        if extents is None:
            return None

        start_time = time.time_ns()
        extent_tree = ExtentTree(BTREE_DEGREE)
        end_time = time.time_ns()
        elapsed_time_microseconds = (end_time - start_time) / 1000 
        print(f"ExtentTree: {elapsed_time_microseconds:.2f} us")

        """ insert extents to extent_tree """
        for extent in extents:
            #start_time = time.time_ns()
            extent_tree.insert(Item(extent.lba, extent))
            #end_time = time.time_ns()
            #elapsed_time_microseconds = (end_time - start_time) / 1000 
            #print(f"extent_tree insert: {elapsed_time_microseconds:.2f} us")

        return extent_tree
    
    def create_or_get_extent_tree(self, dev_id, ino, file_size):
        extent_tree = None 
        value = self._lru_cache.get((dev_id, ino))
        if value is None:
            extent_tree = self.build_tree(dev_id, ino)
            if extent_tree:
                ev_key = self._lru_cache.put((dev_id, ino), 
                        (file_size, extent_tree))
                if ev_key:
                    self._fpath_dict.pop((ev_key[0], ev_key[1]), None)
        else:
            """ check the file modification """
            if value[0] != file_size:
                #print("extent modified: ino={}" .format(ino))
                ret_extent_tree = self._lru_cache.delete((dev_id, ino))
                if ret_extent_tree:
                    del ret_extent_tree

                extent_tree = self.build_tree(dev_id, ino)
                if extent_tree:
                    ev_key = self._lru_cache.put((dev_id, ino), 
                            (file_size, extent_tree))
                    if ev_key:
                        self._fpath_dict.pop((ev_key[0], ev_key[1]), None)
            else:
                extent_tree = value[1]

        return extent_tree

    def handle_readpages(self, readpages_info):
        extent_tree = self.create_or_get_extent_tree(readpages_info.dev_id,
                readpages_info.ino, readpages_info.file_size)
        if extent_tree is None:
            return None

        extent = None

        for i in range(readpages_info.size):
            index = readpages_info.indexes[i]

            if extent and extent_hold_key(extent, index << 12):
                is_extent_hold_lba = True
            else:
                item = extent_tree.find(Item(index << 12))
                if item:
                    extent = item.v
                else:
                    #print("i={}, extent is none {}".format(i, index))
                    continue

                is_extent_hold_lba = True

            if extent.lba > 0:
                index = index % (extent.lba >> 12)

            extent.reset_ref_cnt(index)
            extent.is_readaheads[index] = readpages_info.is_readaheads[i]


    def handle_page_reference(self, page_info):
        extent_tree = self.create_or_get_extent_tree(page_info.dev_id, 
                page_info.ino, page_info.file_size)
        if extent_tree is None:
            return None 
        
        index = page_info.index
        item = extent_tree.find(Item(index << 12))
        if item:
            extent = item.v

            if extent.lba > 0:
                index = index % (extent.lba >> 12)
            extent.add_ref_cnt(index)

    def handle_vfs_unlink(self, page_info):
        ret_extent_tree = self._lru_cache.delete((page_info.dev_id, page_info.ino))
        if ret_extent_tree:
            del ret_extent_tree

    def evaluate_extent_readahead(self, extent, index):
        if extent.lba > 0:
            start_idx = index % (extent.lba >> 12)
        else:
            start_idx = index 
        
        if self._steering_on > 0:
            cnt = 0
            """ check next several index """
            end_idx = min((extent.length >> 12) - 1, start_idx + (MAX_EXTENT_SIZE << 12) - 1)
            for i in range(start_idx, end_idx + 1):
                """ bypass readahead and one time access """
                if extent.is_readaheads[i] == 1 and \
                        extent.ref_cnts[i] == 1:
                            cnt += 1

            length = end_idx - start_idx + 1
            ratio = 100 * cnt / length
            #ratio = 0 # XXX: test
            translator_debug("start={}, end={}, cnt={}, length={}, ratio={}, extent={}" 
                    .format(start_idx, end_idx, cnt, length, ratio, extent))

            if ratio > 50:
                return -1
        else:
            if extent.is_readaheads[start_idx] == 1 and \
                    extent.ref_cnts[start_idx] == 1:
                    print("readahead {} and ref_cnts is {}, start_idx={}, index={}" 
                            .format(extent.is_readaheads[start_idx], 
                                extent.ref_cnts[start_idx], start_idx, index))
                    return -1
        return 0

    def evaluate_extent_ref_cnt(self, extent, index, pba_list):
        if extent.lba > 0:
            start_idx = index % (extent.lba >> 12)
        else:
            start_idx = index 
        
        pba_list = []
        cont = False
        end_idx = min(len(extent.ref_cnts) - 1, start_idx + (MAX_EXTENT_SIZE >> 12) - 1)
        
        avg_ref_cnt = extent.get_avg_ref_cnt()
        """ find contiguous pba which has hot extent range """
        while start_idx < end_idx:
            tmp_list = []
            for i in range(start_idx, end_idx + 1):
                start_idx += 1
                
                if extent.ref_cnts[i] <= avg_ref_cnt:
                    if cont is False:
                        continue
                    else:
                        break

                tmp_list.append(extent.pba + (i << 12))
                cont = True
            
            if len(tmp_list):
                pba_list.append((tmp_list[0], len(tmp_list)))

        translator_debug(pba_list)

        return pba_list

    def trans_pba(self, page_info):
        dev_id = page_info.dev_id
        ino = page_info.ino
        index = page_info.index
        file_offset = index << 12
        file_size = page_info.file_size
        pba_list = []

        extent_tree = self.create_or_get_extent_tree(dev_id, ino, file_size)
        if extent_tree:
            item = extent_tree.find(Item(file_offset))
            if item:
                extent = item.v
                """ cold cond1: readpages but not accessed, cond2: adaptive range """
                if True:
                    ret = self.evaluate_extent_readahead(extent, index)
                    if ret:
                        return pba_list

                    if self._steering_on > 0:
                        return self.evaluate_extent_ref_cnt(extent, index, pba_list)

                else:
                    pba_list.append((extent.pba + (file_offset - extent.lba), -1))
                Stats.ext_found += 1
                return pba_list 
            else:
                Stats.ext_not_found += 1
                pass

        return pba_list
