import socket
import os
import struct
import time 

socket_path = "/tmp/my_unix_socket"
page_info = {"event_type": 1, "dev_id": 123, "ino": 456, "index": 789, "file_size": 1024}

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
    try:
        os.remove(socket_path)
    except OSError:
        pass

    s.bind(socket_path)
    s.listen()

    print("Waiting for connection...")
    connection, client_address = s.accept()
    
    num_iter = 1000 * 1000
    try:
        print("Connected to", client_address)
        start_time = time.time_ns()
        for i in range(num_iter):
            packed_data = struct.pack('iiiii', page_info["event_type"], page_info["dev_id"], page_info["ino"], page_info["index"], page_info["file_size"])
            connection.sendall(packed_data)
 
        elapsed = time.time_ns() - start_time
        print(elapsed)
       
        print("end of sending", elapsed / num_iter, "num_iter=", num_iter)
        time.sleep(100)
    finally:
        # 연결 종료 후 소켓 파일 삭제
        connection.close()
        os.remove(socket_path)

print("Data sent successfully.")
