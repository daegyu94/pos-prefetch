import socket
import os
import struct
import time 

# Unix Domain Socket 파일 경로
socket_path = "/tmp/my_unix_socket"

import struct

class BPFEvent:
    def __init__(self, event_type, dev_id, ino, filepath):
        self.event_type = event_type
        self.dev_id = dev_id
        self.ino = ino
        self.filepath = filepath

    def pack(self):
        filepath_bytes = self.filepath.encode('utf-8')
        format_string = 'I I Q 256s'
        packed_data = struct.pack(format_string,
                self.event_type,
                self.dev_id,
                self.ino, 
                filepath_bytes)
        return packed_data

my_data = BPFEvent(
    event_type=1,
    dev_id=123,
    ino=456,
    filepath="abcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefgabcdefg"
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
    
    #num_iters = 5
    #num_iters = 50
    #num_iters = 1000
    num_iters = 1000 * 1000
    try:
        print("Connected to", client_address)
        start_time = time.time_ns()
        for i in range(num_iters):
            packed_data = my_data.pack()

            connection.sendall(packed_data)
 
        elapsed = time.time_ns() - start_time
        print(num_iters, elapsed)
       
        print("end of sending: avg(ns)=", elapsed / num_iters)
        time.sleep(100)
    finally:
        connection.close()
        os.remove(socket_path)

print("Data sent successfully.")
