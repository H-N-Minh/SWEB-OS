#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "assert.h"

int mmap4() {
    // open and mmap
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap failed\n");
        return 1;
    }

    // printf("mmap done, writting to mapped page\n");

    int *num = (int*) addr;
    *num = 10;
    // printf("done writting, forking now\n");
    if (fork() == 0) {
        // This is the child process.
        assert(*num == 10 && "Error: Child sees different number after mmap then fork\n");
        // printf("child starts\n");
        // printf("Child sees *num = %d\n", *num);
        *num = 20;
        // printf("Child changed *num to 20\n");
        exit(0);
    } else {
        // printf("parent starts\n");
        // This is the parent process.
        sleep(1);  // Sleep for a second to let the child run.
        
        if (*num != 20) {
            printf("Error: Parent sees *num = %d, but it should be 20\n", *num);
            return -1;
        }
        // printf("Parent sees *num = %d\n", *num);
    }

    close(fd); // Close the file descriptor when done.

    return 0;
}
