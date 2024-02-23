import mmap
import os
import struct
import posix_ipc
import time

class Data:
    def __init__(self):
        self.values = [0] * 5

file_path = "shared_memory.bin"
file_size = 20

# Create or open semaphores
sem = posix_ipc.Semaphore("my_semaphore2", posix_ipc.O_CREAT, initial_value=1)

with open(file_path, 'wb') as f:
    f.write(b'\x00' * file_size)

with open(file_path, 'r+b') as f:
    shared_memory = mmap.mmap(f.fileno(), file_size, access=mmap.ACCESS_WRITE)

    num_iters = 10 * 1000 * 1000

    s = time.time()
    for i in range(num_iters):
        sem.acquire()  # Wait for C++ to finish reading

        data = Data()
        data.values = [i] * 5

        packed_data = struct.pack('IIIII', *data.values)
        shared_memory.seek(0)
        shared_memory.write(packed_data)

    elapsed = time.time() - s

    print(num_iters, elapsed, elapsed / num_iters)

sem.unlink()

