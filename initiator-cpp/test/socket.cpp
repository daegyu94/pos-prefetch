#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "socket.h"
#include "my_types.h"

SocketClient::SocketClient(const char* socket_path, EventQueue &queue) : 
	socket_path(socket_path), server_socket(-1), event_queue(queue) {}

SocketClient::~SocketClient() {
    if (server_socket != -1) {
        close(server_socket);
    }
    Stop();
}

bool SocketClient::Connect() {
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un server_address{};
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, socket_path, sizeof(server_address.sun_path) - 1);
	int retry_cnt = 0;

	while (1) {
		if (connect(server_socket, reinterpret_cast<struct sockaddr*>(&server_address),
					sizeof(server_address)) < 0) {
			printf("[WARNING] Failed to connect to the python program: %d\n", 
					retry_cnt);
		} else {
            break;
        }

		if (retry_cnt++ == 5) {
			return false;
		}
		
		sleep(1);
	}

    return true;
}

void SocketClient::Start() {
    thd = std::thread(&SocketClient::ReceiveEvent, this);
}

void SocketClient::Stop() {
    is_running = false;
    event_queue.Stop();
    if (thd.joinable()) {
        thd.join();
    }
}

void SocketClient::ReceiveEvent() {
    while (true) {
        
#if 0
        PageInfo received_page_info;
        ssize_t bytes_received = recv(server_socket, &received_page_info,
				sizeof(received_page_info), 0);

        if (bytes_received < 0) {
            std::cerr << "Error receiving data." << std::endl;
            break; // Exit the loop on error
        } else if (bytes_received == 0) {
            std::cout << "Connection closed by the server." << std::endl;
            break; // Exit the loop when the connection is closed
        } else {
            // insert to event_queue
            EventList* event_list = new EventList;
            event_queue.Enqueue(event_list); 
        }
#endif
    }

    close(server_socket);
}

