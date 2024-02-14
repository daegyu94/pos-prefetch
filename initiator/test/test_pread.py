import os
import subprocess

from ioctl import *
from extent import *

max_extent_size = 128 * 1024

def drop_caches():
    command = "echo 3 > /proc/sys/vm/drop_caches"
    result = subprocess.run(command, shell=True, check=True)
    if result.returncode == 0:
        print("Success to execute {}" .format(command))
    else:
        print("Failed to execute {}" .format(command))
    
def pread_example(file_path, offset, size):
    print(file_path, offset, size)
    
    fd = os.open(file_path, os.O_RDONLY)
    if fd <= 0:
        print("Failed to open file")

    read_bytes = os.pread(fd, size, offset)
    
    os.close(fd)

def main():
    file_path = '/mnt/nvme0/dummyfile_130M'
    
    extents = Fiemap(file_path).do()
    
    extent = extents[0]
    offset = extent.lba + extent.length - (max_extent_size // 2) 
    size = max_extent_size

    pread_example(file_path, offset, size)
    pread_example(file_path, offset, size)
    
    drop_caches()

if __name__ == "__main__":
    main()

