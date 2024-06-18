#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

// TODO: currently close(fd) is never called

int shared9() {
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    /////////////////////////////////////////////// TEST 9 ////////////////////////////////////////////////
    // Child writes to the shared page, then exits without munmap. Parent should still be able to see what the child wrote. This shows the page is written back to file (even without munmap) when process terminates
    void* addr = MAP_FAILED;
    addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 9 failed\n");
        return 1;
    }
    if (close(fd) == -1)
    {
      printf("Close failed\n");
      return 1;
    }
    pid_t child_pid = -1;
    int child_status = -1;
    char *st = "test 9";
    child_pid = fork();
    if (child_pid == 0) {
        // write st to the page, then exits without mmunmap
        memcpy(addr, st, strlen(st) + 1);
        exit(0);
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status != 0)
        {
            printf("Error fork 9: Child process did not finish successfully (code %d)\n", child_status);
            return -1;
        }
        // check if the parent can see what the child wrote
        if (memcmp(addr, st, strlen(st))) {
            printf("Error fork 9: the data the child wrote should be written onto the file after child died\n");
            return -1;
        }
    }
    // reset the file data and test munmap
    memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
    if (munmap(addr, 4096))
    {
        printf("munmap 8 failed\n");
        return -1;
    }

    return 0;
}
