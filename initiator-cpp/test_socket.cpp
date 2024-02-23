#include <iostream>

#include "socket.h"


int main() {
#define SOCKET_PATH "/tmp/my_unix_socket"
    const char *socket_path = SOCKET_PATH;
    SocketClient client(socket_path);

    if (client.Connect()) {
        client.ReceiveData(); // entry point of program
    } else {
        printf("[ERROR] Failed to connect to the python program\n");
    }

    return 0;
}
