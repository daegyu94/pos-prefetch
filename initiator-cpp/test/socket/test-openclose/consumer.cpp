#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <queue>
#include <arpa/inet.h>

using namespace std;

struct BPFEvent {
    uint32_t event_type;
    uint32_t dev_id;
    uint64_t ino;
    char filepath[256];
} __attribute__ ((packed));


void print_bits(uint32_t num) {
    int i;
    int size = sizeof(uint32_t) * 8; // Number of bits in uint32_t

    for (i = size - 1; i >= 0; i--) {
        uint32_t mask = 1U << i;
        printf("%c", (num & mask) ? '1' : '0');

        if (i % 8 == 0 && i != 0) {
            printf(" "); // Add a space every 8 bits for better readability
        }
    }
    printf("\n");
}

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
    
    printf("sizeof BPFEvent=%lu\n", sizeof(struct BPFEvent));

    while (1) {
		struct BPFEvent event;
        ssize_t bytes_received = recv(server_socket, &event, sizeof(event), 0);
        if (bytes_received < 0) {
            std::cerr << "Error receiving data." << std::endl;
        } else if (bytes_received == 0) {
            break;
        } else {
            uint32_t event_type = event.event_type;
            
            //print_BPF_event(event);  
            
            if (event_type == 1) {
                //print_page_event(event.page_event);  
            } else if (event_type == 2 ) {
                //print_readpages_event(event.readpages_event);  
            } else {
                printf("[ERR] event_type=%u\n", event_type);
            }
        }
    }
    
    close(server_socket);

    return 0;
}
