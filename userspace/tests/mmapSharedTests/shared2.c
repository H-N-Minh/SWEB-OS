#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"


int shared2() {
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    //////////////////////////////////////////////// TEST 2 ////////////////////////////////////////////////
    // mmap with read-only protection, child tries to write, parent expects child to crash
    void *addr = MAP_FAILED;
    addr = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 2 failed\n");
        return 1;
    }
    // test reading
    if (memcmp(addr, "can u see me now", strlen("can u see me now") + 1))
    {
        printf("Error: reading 2 failed\n");
        return -1;
    }
    // test IPC
    pid_t child_pid = -1;
    int child_status = -1;
    int *num = (int*) addr;
    child_pid = fork();
    if (child_pid == 0) {
        *num = 20;
        exit(0);
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status == 0)
        {
            printf("Error fork 2: Child process did not crash as expected (code %d)\n", child_status);
            return -1;
        }
    }
    // reset the file data and test munmap
    if (munmap(addr, 4096))
    {
        printf("munmap 2 failed\n");
        return -1;
    }

    return 0;
}
