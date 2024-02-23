import socket
import os
import struct
import time 

# Unix Domain Socket 파일 경로
socket_path = "/tmp/my_unix_socket"

import struct

class PageInfo:
    def __init__(self, event_type, dev_id, ino, index, file_size):
        self.event_type = event_type  # uint8_t
        self.dev_id = dev_id  # uint32_t
        self.ino = ino  # uint64_t
        self.index = index  # uint64_t
        self.file_size = file_size  # uint64_t

    def pack(self):
        # 패킹 포맷 문자열 생성
        format_string = 'B I Q Q Q'

        # 데이터 패킹
        packed_data = struct.pack(format_string,
                                  self.event_type,
                                  self.dev_id,
                                  self.ino,
                                  self.index,
                                  self.file_size)

        return packed_data


class ReadPagesData:
    def __init__(self, event_type, dev_id, ino, file_size, indexes, is_readaheads, size):
        self.event_type = event_type
        self.dev_id = dev_id
        self.ino = ino
        self.file_size = file_size
        self.indexes = indexes
        self.is_readaheads = is_readaheads
        self.size = size

    def pack(self):
        # 패킹 포맷 문자열 생성
        format_string = 'B I Q Q ' + ' '.join(['Q'] * 32) + ' ' + 'B' * 32 + ' I'

        # 데이터 패킹
        packed_data = struct.pack(format_string,
                                  self.event_type,
                                  self.dev_id,
                                  self.ino,
                                  self.file_size,
                                  *self.indexes,
                                  *self.is_readaheads,
                                  self.size)

        return packed_data


# 테스트 데이터 생성
page_info = PageInfo(event_type=1, dev_id=2, ino=3, index=4, file_size=5)
read_pages_data = ReadPagesData(event_type=2, dev_id=2, ino=3, file_size=5,
                                indexes=[10, 20, 30] + [0] * 29,
                                is_readaheads=[1, 0, 1] + [0] * 29,
                                size=4096)

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
    try:
        os.remove(socket_path)
    except OSError:
        pass

    s.bind(socket_path)
    s.listen()

    print("Waiting for connection...")
    connection, client_address = s.accept()
    
    iter = 4
    try:
        print("Connected to", client_address)
        start_time = time.time_ns()
        for i in range(iter):
            packed_data = page_info.pack()
            #if i % 2:
            #    packed_data = page_info.pack()
            #else:
            #    packed_data = read_pages_data.pack()

            connection.sendall(packed_data)
 
        elapsed = time.time_ns() - start_time
        print(elapsed)
       
        print("end of sending", elapsed / iter)
        time.sleep(100)
    finally:
        connection.close()
        os.remove(socket_path)

print("Data sent successfully.")
