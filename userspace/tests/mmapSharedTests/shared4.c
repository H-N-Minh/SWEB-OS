#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

// TODO: currently close(fd) is never called

int shared4() {
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    /////////////////////////////////////////////// TEST 4 ////////////////////////////////////////////////
    // Child calls munmap, parent should still be able to access the page and then call munmap itself on the same page
    void* addr = MAP_FAILED;
    addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 4 failed\n");
        return 1;
    }
    // test IPC
    pid_t child_pid = -1;
    int child_status = -1;
    char *st = "test 4";
    child_pid = fork();
    if (child_pid == 0) {
        // write st to the page
        memcpy(addr, st, strlen(st) + 1);
        if (munmap(addr, 4096))
        {
            printf("munmap 4 failed\n");
            return -1;
        }
        exit(0);
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status != 0)
        {
            printf("Error fork 4: Child process did not finish successfully (code %d)\n", child_status);
            return -1;
        }
        // read the page to check if the write was successful
        if (memcmp(addr, st, strlen(st))) {
            printf("Error: reading/writing 4 failed\n");
            return -1;
        }
        if (munmap(addr, 4096))
        {
            printf("munmap 4 failed\n");
            return -1;
        }
    }

    return 0;
}
