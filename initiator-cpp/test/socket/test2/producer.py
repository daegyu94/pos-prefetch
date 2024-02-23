import socket
import os
import struct
import time 

# Unix Domain Socket 파일 경로
socket_path = "/tmp/my_unix_socket"

import struct

class PageEvent:
    def __init__(self, event_type, dev_id, ino, file_size, index):
        self.event_type = event_type  # uint8_t
        self.dev_id = dev_id  # uint32_t
        self.ino = ino  # uint64_t
        self.file_size = file_size  # uint64_t
        self.index = index  # uint64_t

    def pack(self):
        format_string = 'I I Q Q Q'
        packed_data = struct.pack(format_string,
                                  self.event_type,
                                  self.dev_id,
                                  self.ino,
                                  self.file_size,
                                  self.index)

        #packed_size = struct.calcsize(format_string)
        #print(f"PageEvent Packed Data Size: {packed_size} bytes")

        return packed_data

page_data = PageEvent(event_type=1, dev_id=2, ino=3, file_size=4, index=5)

class ReadPagesEvent:
    def __init__(self, event_type, dev_id, ino, file_size, indexes, is_readaheads, size):
        self.event_type = event_type
        self.dev_id = dev_id
        self.ino = ino
        self.file_size = file_size
        self.indexes = indexes
        self.is_readaheads = is_readaheads
        self.size = size

    def pack(self):
        format_string = 'I I Q Q ' + ' '.join(['Q'] * 32) + ' ' + ' '.join(['B'] * 32) + ' I'
        packed_data = struct.pack(format_string,
                                  self.event_type,
                                  self.dev_id,
                                  self.ino,
                                  self.file_size,
                                  *self.indexes,
                                  *self.is_readaheads,
                                  self.size)
        #packed_size = struct.calcsize(format_string)
        #print(f"ReadPagesEvent Packed Data Size: {packed_size} bytes")
        return packed_data

readpages_data = ReadPagesEvent(
    event_type=2,
    dev_id=123,
    ino=456,
    file_size=1024,
    indexes=[10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310, 320],
    is_readaheads=[1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
    size=12345
)


class BPFEvent:
    def __init__(self, event_type, dev_id, ino, file_size, indexes, is_readaheads, size):
        self.event_type = event_type
        self.dev_id = dev_id
        self.ino = ino
        self.file_size = file_size
        self.indexes = indexes
        self.is_readaheads = is_readaheads
        self.size = size

    def pack(self):
        format_string = 'I I Q Q ' + ' '.join(['Q'] * 32) + ' ' + ' '.join(['B'] * 32) + ' I'
        packed_data = struct.pack(format_string,
                                  self.event_type,
                                  self.dev_id,
                                  self.ino,
                                  self.file_size,
                                  *self.indexes,
                                  *self.is_readaheads,
                                  self.size)
        return packed_data

my_data = BPFEvent(
    event_type=1,
    dev_id=123,
    ino=456,
    file_size=1024,
    indexes=[10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310, 320],
    is_readaheads=[1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
    size=12345
)


with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
    try:
        os.remove(socket_path)
    except OSError:
        pass

    s.bind(socket_path)
    s.listen()

    print("Waiting for connection...")
    connection, client_address = s.accept()
    
    #num_iters = 10
    #num_iters = 50
    #num_iters = 1000
    num_iters = 1000 * 1000
    try:
        print("Connected to", client_address)
        start_time = time.time_ns()
        for i in range(num_iters):
            #packed_data = my_data.pack()
            if i % 2:
                packed_data = readpages_data.pack()
            else:
                packed_data = page_data.pack()

            connection.sendall(packed_data)
 
        elapsed = time.time_ns() - start_time
        print(num_iters, elapsed)
       
        print("end of sending: avg(ns)=", elapsed / num_iters)
        time.sleep(100)
    finally:
        connection.close()
        os.remove(socket_path)

print("Data sent successfully.")
