#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

//#define FILE_SIZE (100 * 1024 * 1024) // 100MB
#define FILE_SIZE (128 * 1024)
#define BUFFER_SIZE 4096 // 4KB

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd;
    char buffer[BUFFER_SIZE];
    memset(buffer, 'A', BUFFER_SIZE);

    // 파일 생성 및 100MB 데이터 쓰기
    fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open (write)");
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < FILE_SIZE / BUFFER_SIZE; i++) {
        if (write(fd, buffer, BUFFER_SIZE) != BUFFER_SIZE) {
            perror("write");
            close(fd);
            return EXIT_FAILURE;
        }
    }
    fsync(fd);
    close(fd);

#if 1
    int drop_caches = open("/proc/sys/vm/drop_caches", O_WRONLY);
    if (drop_caches >= 0) {
        write(drop_caches, "3", 1);
        close(drop_caches);
    } else {
        perror("drop_caches");
    }
#endif 

    // 파일 읽기 (4KB씩 파일 끝까지)
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open (read)");
        return EXIT_FAILURE;
    }
    
    while (read(fd, buffer, BUFFER_SIZE) > 0);
    
    //posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    posix_fadvise(fd, 10 * 1024, 33 * 1024, POSIX_FADV_DONTNEED);
    //posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);
    
    close(fd);
    return EXIT_SUCCESS;
}

