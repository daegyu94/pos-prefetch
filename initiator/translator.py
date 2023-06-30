#import threading
import queue 
import asyncio
from functools import lru_cache

from metadata import *
from extent_tree import *
from ioctl import *
from lru_cache import *
from stats import *

class TransInfo:
    def __init__(self, subsys_id, ns_id, pba):
        self.subsys_id = subsys_id
        self.ns_id = ns_id
        self.pba = pba

#class Translator(threading.Thread):
class Translator():
    def __init__(self, meta_dict, fpath_dict):
        """
        threading.Thread.__init__(self)
        super().__init__()
        self.name = "Translator"
        """

        self._queue = queue.Queue()
        self._meta_dict = meta_dict
        self._fpath_dict = fpath_dict
        #                              [0][0]   [0][1]        [1]
        # key: (ino, dev_id), value: ((mtime, file_size), extent_tree)
        self._lru_cache = LRUCache(1024) 
    
    def __del__(self):
        self._queue.join()

    """
    async def handle(self):
        while True:
            info = self._queue.get()
            self._queue.task_done()

            pd_info = info[0]
            grpc_queue = info[1]
            
            #print("translator handle: "
            #        "ino={} lba={}, trans_qsize={}, grpc_qsize={}" 
            #        .format(pd_info.ino, pd_info.lba, 
            #            self._queue.qsize(), grpc_queue.qsize()))

            dev_id = pd_info.dev_id
            ino = pd_info.ino
            lba = pd_info.lba
            mtime = pd_info.mtime
            file_size = pd_info.file_size

            file_path = await self.find_file_path_slow(ino, dev_id)
            if file_path is None:
                continue

            extents = Fiemap(file_path).do()
            if extents is None:
                continue

            extent_tree = ExtentTree(8)
            for extent in extents:
                extent_tree.insert(Item(extent.lba, extent))

            ev_key = self._lru_cache.put((ino, dev_id), 
                    ((mtime, file_size), extent_tree))
            if ev_key:
                self._fpath_dict.pop((ev_key[0], ev_key[1]), None)

            grpc_queue.put(pd_info)

    def run(self):
        asyncio.run(self.handle())

    def get_queue(self):
        return self._queue
    """

    def find_file_path_fast(self, ino, dev_id):
        ret = self._fpath_dict.get((ino, dev_id))
        if ret is None:
            return None

        path_type = ret[0]
        file_path = ret[1].decode('utf-8');

        if path_type == 2:
            mnt_path = self._meta_dict.get_mntpnt(dev_id) 
            return mnt_path + file_path
        else: 
            return file_path

    async def find_file_path_slow(self, ino, dev_id):
        mnt_path = self._meta_dict.get_mntpnt(dev_id) 

        cmd = f"find {mnt_path} -xdev -inum {ino}"
        process = await asyncio.create_subprocess_shell(cmd, 
                stdout=asyncio.subprocess.PIPE, 
                stderr=asyncio.subprocess.PIPE)
        stdout, _ = await process.communicate()
        output = stdout.decode().strip()
        if output:
            return output.split()[0]

    def build_tree(self, ino, dev_id):
        file_path = self.find_file_path_fast(ino, dev_id)
        if file_path is None:
            return None
        
        extents = Fiemap(file_path).do()
        if extents is None:
            return None

        extent_tree = ExtentTree(8)
        # insert extents to extent_tree
        for extent in extents:
            extent_tree.insert(Item(extent.lba, extent))

        return extent_tree
    
    # get cached extent_tree or building extent tree
    def trans_pba(self, info):
        dev_id = info.dev_id
        ino = info.ino
        lba = info.lba
        mtime = info.mtime
        file_size = info.file_size
        
        #print("inode={}, lba={}" .format(ino, lba))
        value = self._lru_cache.get((ino, dev_id))
        if value is None:
            extent_tree = self.build_tree(ino, dev_id)
            if extent_tree:
                ev_key = self._lru_cache.put((ino, dev_id), 
                        ((mtime, file_size), extent_tree))
                if ev_key:
                    self._fpath_dict.pop((ev_key[0], ev_key[1]), None)
        else:
            # check the file modification
            if value[0][0] != mtime and value[0][1] != file_size:
                #print("extent modified: ino={}" .format(ino))
                self._lru_cache.delete((ino, dev_id))
                extent_tree = self.build_tree(ino, dev_id)
                if extent_tree:
                    ev_key = self._lru_cache.put((ino, dev_id), 
                            ((mtime, file_size), extent_tree))
                    if ev_key:
                        self._fpath_dict.pop((ev_key[0], ev_key[1]), None)
            else:
                extent_tree = value[1]
       
        if extent_tree:
            item = extent_tree.find(Item(lba))
            if item:
                extent = item.v
                Stats.ext_found += 1
                return extent.pba + (lba - extent.lba) # pba + offset
            else:
                Stats.ext_not_found += 1
                pass

