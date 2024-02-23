#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <queue>
#include <arpa/inet.h>

using namespace std;

// PageInfo 구조체 정의
struct PageInfo {
    uint8_t event_type;
    uint32_t dev_id;
    uint64_t ino;
    uint64_t index;
    uint64_t file_size;
};

struct ReadPagesInfo {
    uint32_t dev_id;
    uint64_t ino;
    uint64_t file_size;
    uint64_t indexes[32];	
    uint8_t is_readaheads[32];
    uint32_t size;
};

struct ReceivedData {
    struct PageInfo page_info;
};

void printPageInfo(const PageInfo& pageInfo) {
    cout << "Received PageInfo:" << endl;
    cout << "dev_id: " << pageInfo.dev_id << endl;
    cout << "ino: " << pageInfo.ino << endl;
    cout << "index: " << pageInfo.index << endl;
    cout << "file_size: " << pageInfo.file_size << endl;
    cout << endl;
}

void printReadPagesInfo(const ReadPagesInfo& readPagesInfo) {
    cout << "Received ReadPagesInfo:" << endl;
    cout << "dev_id: " << readPagesInfo.dev_id << endl;
    cout << "ino: " << readPagesInfo.ino << endl;
    cout << "file_size: " << readPagesInfo.file_size << endl;
    cout << "indexes: ";
    for (int i = 0; i < 32; ++i) {
        cout << readPagesInfo.indexes[i] << " ";
    }
    cout << endl;
    cout << "is_readaheads: ";
    for (int i = 0; i < 32; ++i) {
        cout << static_cast<int>(readPagesInfo.is_readaheads[i]) << " ";
    }
    cout << endl;
    cout << "size: " << readPagesInfo.size << endl;
    cout << endl;
}

int main() {
    // Unix Domain Socket 파일 경로
    const char* socket_path = "/tmp/my_unix_socket";
    
    queue<struct PageInfo> q;

    // 소켓 생성 및 연결
    int server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un server_address{};
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, socket_path, sizeof(server_address.sun_path) - 1);

    if (connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Error connecting to the server." << std::endl;
        return -1;
    }
    
    printf("sizeof(ReadPagesInfo)=%lu\n", sizeof(struct ReadPagesInfo));
    while (1) {
        struct ReceivedData buf;

        ssize_t bytes_received = recv(server_socket, &buf, sizeof(buf), 0);
        
        if (bytes_received < 0) {
            std::cerr << "Error receiving data." << std::endl;
        } else if (bytes_received == 0) {
            break;
        } else {
            uint8_t event_type = buf.page_info.event_type;
            if (event_type == 1) {
                printPageInfo(buf.page_info);
            } else if (event_type == 2) {
                //printReadPagesInfo(buf.read_pages_info);
            } else {
                printf("[ERR] event_type=%u\n", event_type);
            }
        }
    }

    // 소켓 종료
    close(server_socket);

    return 0;
}
