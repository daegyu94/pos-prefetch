import socket
import os
import struct

# Unix Domain Socket 파일 경로
socket_path = "/tmp/my_unix_socket"

# PageInfo 객체 생성
page_info = {"event_type": 1, "dev_id": 123, "ino": 456, "index": 789, "file_size": 1024}

# 소켓 생성 및 연결
with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
    # 이미 존재하는 소켓 파일이 있다면 삭제
    try:
        os.remove(socket_path)
    except OSError:
        pass

    s.bind(socket_path)
    s.listen()

    print("Waiting for connection...")
    connection, client_address = s.accept()

    try:
        print("Connected to", client_address)

        # 데이터 패킹 및 전송
        packed_data = struct.pack('iiiii', page_info["event_type"], page_info["dev_id"], page_info["ino"], page_info["index"], page_info["file_size"])
        connection.sendall(packed_data)

    finally:
        # 연결 종료 후 소켓 파일 삭제
        connection.close()
        os.remove(socket_path)

print("Data sent successfully.")
