#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() 
{
    // open and mmap
    int fd = open("usr/testmmn.txt", O_WRONLY | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }
    void* addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);  // Map 1 page of memory
    if (addr == (void*) -1) {
        printf("mmap failed\n");
        return 1;
    }
    close(fd);

    printf("Memory mapped at address %p\n", addr);
    *(size_t*)addr = 69;
    printf("Value at address %p: %zu\n", addr, *(size_t*)addr);

    // write to the memory
    const char* st = "Hello, world!";  // String to write to the mapped memory
    memcpy(addr, st, strlen(st));  // Copy the string to the mapped memory



    // check if the data was written to the memory
    fd = open("usr/testmmn.txt", O_RDONLY);
    char buffer[100];
    ssize_t bytes_read = read(fd, buffer, 20);
    buffer[bytes_read] = '\0';
    printf("Content of the file: %s\n", buffer);
    close(fd);


    munmap(addr, 4096);  // Unmap the memory
    return 0;
}