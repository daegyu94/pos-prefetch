#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <queue>
#include <arpa/inet.h>

using namespace std;

struct PageEvent {
    uint32_t dev_id;
    uint64_t ino;
    uint64_t file_size;
    uint64_t indexes[0];	
} __attribute__ ((packed));

struct ReadPagesEvent {
    uint32_t dev_id;
    uint64_t ino;
    uint64_t file_size;
    uint64_t indexes[32];	
    uint8_t is_readaheads[32];
    uint32_t size;
} __attribute__ ((packed));

struct RecvBuffer {
    uint32_t event_type;
    union {
        struct PageEvent page_event; 
        struct ReadPagesEvent readpages_event; 
    };
};

struct BPFEvent {
    uint32_t event_type;
    uint32_t dev_id;
    uint64_t ino;
    uint64_t file_size;
    uint64_t indexes[32];	
    uint8_t is_readaheads[32];
    uint32_t size;
} __attribute__ ((packed));


void print_page_event(const PageEvent& event) {
	printf("dev_id=%u, ino=%lu, file_size=%lu\n",
			event.dev_id, event.ino, event.file_size);

	printf("Indexes: ");
	for (int i = 0; i < 1; ++i) {
		printf("%lu ", event.indexes[i]);
	}
	printf("\n");
	printf("\n");
}

void print_readpages_event(const ReadPagesEvent& event) {
	printf("dev_id=%u, ino=%lu, file_size=%lu\n",
			event.dev_id, event.ino, event.file_size);

	printf("Indexes: ");
	for (int i = 0; i < 32; ++i) {
		printf("%lu ", event.indexes[i]);
	}
	printf("\n");

	printf("Is Readaheads: ");
	for (int i = 0; i < 32; ++i) {
		printf("%u ", event.is_readaheads[i]);
	}
	printf("\n");

	printf("Size: %u\n", event.size);
	printf("\n");
}


void print_BPFEvent(const BPFEvent& event) {
	printf("event_type=%u, dev_id=%u, ino=%lu, file_size=%lu\n",
			event.event_type, event.dev_id, event.ino, event.file_size);

	printf("Indexes: ");
	for (int i = 0; i < 32; ++i) {
		printf("%lu ", event.indexes[i]);
	}
	printf("\n");

	printf("Is Readaheads: ");
	for (int i = 0; i < 32; ++i) {
		printf("%u ", event.is_readaheads[i]);
	}
	printf("\n");

	printf("Size: %u\n", event.size);
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
    
    printf("sizeof PageEvent=%lu, ReadPages=%lu, RecvBuffer=%lu, BPFEvent=%lu\n", 
            sizeof(struct PageEvent), sizeof(struct ReadPagesEvent), sizeof(struct RecvBuffer),
            sizeof(struct BPFEvent));

    /*
    while (1) {
		struct BPFEvent event;

        ssize_t bytes_received = recv(server_socket, &event, sizeof(event), 0);
        
        if (bytes_received < 0) {
            std::cerr << "Error receiving data." << std::endl;
        } else if (bytes_received == 0) {
            break;
        } else {
            //print_BPFEvent(event);
        }
    }
    */

    while (1) {
		struct BPFEvent event;
        ssize_t bytes_received = recv(server_socket, &event, sizeof(event), 0);
        if (bytes_received < 0) {
            std::cerr << "Error receiving data." << std::endl;
        } else if (bytes_received == 0) {
            break;
        } else {
            uint32_t event_type = event.event_type;
            if (event_type == 1) {
                //print_page_event(event.page_event);  
            } else if (event_type == 2 ) {
                //print_readpages_event(event.readpages_event);  
            } else {
                printf("[ERR] event_type=%u\n", event_type);
            }
        }
        /*
		struct RecvBuffer event;
        ssize_t bytes_received = recv(server_socket, &event, sizeof(event), 0);
        if (bytes_received < 0) {
            std::cerr << "Error receiving data." << std::endl;
        } else if (bytes_received == 0) {
            break;
        } else {
            uint32_t event_type = event.event_type;
            if (event_type == 1) {
                //print_page_event(event.page_event);  
            } else if (event_type == 2 ) {
                //print_readpages_event(event.readpages_event);  
            } else {
                printf("[ERR] event_type=%u\n", event_type);
            }
        }
        */
    }
    
    close(server_socket);

    return 0;
}
