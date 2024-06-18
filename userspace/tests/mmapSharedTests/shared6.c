#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

// TODO: currently close(fd) is never called

int shared6() {
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    /////////////////////////////////////////////// TEST 6 ////////////////////////////////////////////////
    // Child calls munmap, then tries to write to page, child should crashes.
    void* addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 6 failed\n");
        return 1;
    }
    if (close(fd) == -1)
    {
      printf("Close failed\n");
      return 1;
    }
    // test IPC
    pid_t child_pid = -1;
    int child_status = -1;
    char *st = "test 6";
    child_pid = fork();
    if (child_pid == 0) {
        // write st to the page
        if (munmap(addr, 4096))
        {
            printf("munmap 6 failed\n");
            return -1;
        }
        // try to write to the page after munmap
        memcpy(addr, st, strlen(st) + 1);
        exit(0);
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status == 0)
        {
            printf("Error fork 6: Child process did not crash as expected (code %d)\n", child_status);
            return -1;
        }
        if (munmap(addr, 4096))
        {
            printf("munmap 6 failed\n");
            return -1;
        }
    }

    return 0;
}
