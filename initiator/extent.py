import statistics

from my_types import *

extent_debug_on = True
def extent_debug(*args):
    if extent_debug_on:
        func_name = inspect.currentframe().f_back.f_code.co_name
        print("[DEBUG] Extent: {}: {}".format(func_name, " ".join(map(str, args))))


class Extent:
    def __init__(self, lba, pba, length):
        self.lba = lba
        self.pba = pba
        self.length = length

        self.sum_ref_cnt = 0
        self.ref_cnts = [0] * (length >> 12)
        self.is_readaheads = [0] * (length >> 12)
         
    def __repr__(self):
        return "(Extent: lba={}, pba={}, length={},\nref_cnts[]={},\nis_readaheads=[]={})" \
                .format(self.lba, self.pba, self.length, 
                self.ref_cnts, self.is_readaheads)

    def get_max_ref_cnt(self):
        return max(self.ref_cnts)

    def get_stdev_ref_cnt(self):
        if len(self.ref_cnts) >= 2:
            return statistics.stdev(self.ref_cnts)
        else:
            return -1

    def get_avg_ref_cnt(self):
        return self.sum_ref_cnt / (self.length >> 12)
        
    def add_ref_cnt(self, i):
        self.ref_cnts[i] += 1
        self.sum_ref_cnt += 1

    def reset_ref_cnt(self, i):
        self.sum_ref_cnt -= self.ref_cnts[i]
        if self.sum_ref_cnt < 0:
            print("[ERROR] extent sum_ref_cnt={}" .format(self.sum_ref_cnt))
        self.ref_cnts[i] = 0 

def extent_hold_key(extent, pos):
    return extent.lba <= pos and pos <= (extent.lba + extent.length - 1) 

def gen_extent_list(file_size):
    import random
    import numpy as np

    np.random.seed(0)

    page_size = 4096
    mean = 4 * page_size
    std_dev = page_size
    lengths = np.random.normal(mean, std_dev, size=10000)
    # round to nearest multiple of 4096
    lengths = (lengths // page_size) * page_size  
    
    extents = []
    lba = 0
    total_length = 0
    done = 0
    for i, length in enumerate(lengths):
        total_length += int(length)
        
        if (total_length >= file_size):
            if (total_length > file_size):
                old_total_length = total_length - int(length)
                length = file_size - old_total_length
                total_length = old_total_length + length
            done = 1

        extent = Extent(lba, lba, int(length))
        extents.append(extent)
        #print(f"Extent {i}: LBA={extent.lba}, PBA={extent.pba}, 
        #        Length={extent.length}")

        if (done):
            break

        lba += int(length)

    return extents


def main():
    file_size = 1 << 20
    
    extents = gen_extent_list(file_size)

    # Print extents
    total_length = 0
    for extent in extents:
        total_length += extent.length
        print(f"LBA: {extent.lba}, PBA: {extent.pba}, Length: {extent.length}")
    
    if (file_size != total_length):
        print("file_size != total_length {} {}" .format(file_size, total_length))

if __name__ == "__main__":
    main()
