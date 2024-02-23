#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <semaphore.h>

class Data {
public:
    uint32_t values[5];
};

int main() {
    const char* file_path = "shared_memory.bin";
    int fd = open(file_path, O_RDONLY);
    size_t file_size = sizeof(Data);

    Data* shared_memory = static_cast<Data*>(
        mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0)
    );
    close(fd);

    // Open semaphore
    sem_t* sem = sem_open("my_semaphore2", O_RDWR);
    if (!sem) {
        std::cerr << "Failed to sem_open()" << std::endl;
        return -1;
    }

    while (1) {
        #if 0
        for (int i = 0; i < 5; ++i) {
            std::cout << "Read Value[" << i << "]: " << shared_memory->values[i] << std::endl;
        }
        #endif
        sem_post(sem); // Notify the end of the critical section
    }

    sem_close(sem);

    munmap(shared_memory, file_size);

    return 0;
}

