#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// PageInfo 구조체 정의
struct PageInfo {
    int event_type;
    int dev_id;
    int ino;
    int index;
    int file_size;
};

int main() {
    // Unix Domain Socket 파일 경로
    const char* socket_path = "/tmp/my_unix_socket";

    // 소켓 생성 및 연결
    int server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un server_address{};
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, socket_path, sizeof(server_address.sun_path) - 1);

    if (connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error connecting to the server." << std::endl;
        return -1;
    }

    // 데이터 수신
    PageInfo received_page_info;
    ssize_t bytes_received = recv(server_socket, &received_page_info, sizeof(received_page_info), 0);

    if (bytes_received < 0) {
        std::cerr << "Error receiving data." << std::endl;
    } else {
        // 수신된 데이터 출력
        std::cout << "Event Type: " << received_page_info.event_type << std::endl;
        std::cout << "Dev ID: " << received_page_info.dev_id << std::endl;
        std::cout << "Ino: " << received_page_info.ino << std::endl;
        std::cout << "Index: " << received_page_info.index << std::endl;
        std::cout << "File Size: " << received_page_info.file_size << std::endl;
    }

    // 소켓 종료
    close(server_socket);

    return 0;
}

