#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int mmap3() 
{
    // open and mmap
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
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

    // goal: read the content of the file through a virtual pointer addr
    printf("Memory mapped at address %p\n", addr);
    printf("content of the memory: %s\n", (char*)addr);

    // write to the memory
    const char* st = "Hello, world!";  // String to write to the mapped memory
    memcpy(addr, st, strlen(st));  // Copy the string to the mapped memory

    printf("String written to memory: %s\n", (char*)addr);


    // check if the data was written to the memory
    fd = open("usr/testmmn.txt", O_RDONLY);
    char buffer[20];
    ssize_t bytesRead = read(fd, buffer, 20);
    if (bytesRead == -1) {
        printf("read failed\n");
        close(fd);
        return 1;
    }
    buffer[25] = '\0';  // Null-terminate the string
    close(fd);
    printf("Read from file: %s\n", buffer);

    munmap(addr, 4096);  // Unmap the memory
    return 0;
}

// int mmap3() {
//     // open the file
//     int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
//     if (fd == -1) {
//         printf("open failed\n");
//         return 1;
//     }
//     printf("File opened\n");
    
//     // read from the file
//     char buffer[20];
//     ssize_t bytesRead = read(fd, buffer, 20);
//     if (bytesRead == -1) {
//         printf("read failed\n");
//         close(fd);
//         return 1;
//     }
//     printf("file read successful\n");
//     // buffer[25] = '\0';  // Null-terminate the string
    
//     // close the file
//     close(fd);
    
//     printf("Read from file: %s\n", buffer);
    
//     return 0;

// }