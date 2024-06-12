#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int switcher = 0;

int mmap3() 
{
    switcher = 1;

    if (switcher)
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

        printf("fd is %d, vpn is %zu\n", fd, (size_t)addr / 4096);

        // goal: read the content of the file through a virtual pointer addr
        printf("Memory mapped at address %p\n", addr);

        // char buffer[1001];
        // memcpy(buffer, addr, 1000);
        // buffer[1000] = '\0';
        // printf("content of the memory buffer: %s\n", buffer);
        printf("content of the memory addr: %s\n", (char*)addr);

        // write to the memory
        const char* st = "Hello, world!";  // String to write to the mapped memory
        memcpy(addr, st, strlen(st));  // Copy the string to the mapped memory

        printf("content of the memory addr after written to: %s\n", (char*)addr);

        munmap(addr, 4096);  // Unmap the memory
        close(fd);

        printf("closing file and open again to read the content, it should be the same\n");

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

        return 0;
    }
    else    // this is just an example of reading from a file normally, to compare with the mmap
    {
        // open the file
        int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
        if (fd == -1) {
            printf("open failed\n");
            return 1;
        }
        printf("File opened\n");

        // Write to the file
        const char* st = "Hello";
        ssize_t bytesWritten = write(fd, st, strlen(st));
        if (bytesWritten == -1) {
            close(fd);
            return 1;
        }
        printf("File written\n");

        // Move the file pointer back to the beginning of the file
        if (lseek(fd, 0, SEEK_SET) == -1) {
            close(fd);
            return 1;
        }
        
        // read from the file
        char buffer[500];
        ssize_t bytesRead = read(fd, buffer, 500);
        if (bytesRead == -1) {
            printf("read failed\n");
            close(fd);
            return 1;
        }
        printf("file read successful\n");
        // buffer[25] = '\0';  // Null-terminate the string
        
        // close the file
        close(fd);
        
        printf("Read from file: %s\n", buffer);
        
        return 0;

    }
}