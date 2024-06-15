#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

int anonym_shared8() {
    /////////////////////////////////////////////// TEST 8 ////////////////////////////////////////////////
    // Child munmaps the first page, writes to the other 2 pages, then munmaps the other 2 pages. child should exit successfully
    void* addr = MAP_FAILED;
    int num_pages = 20;
    addr = mmap(NULL, num_pages * 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 8 failed\n");
        return 1;
    }
    // test IPC
    pid_t child_pid = -1;
    int child_status = -1;
    char *st = "test 8";
    child_pid = fork();
    if (child_pid == 0) {
        // munmap the first page
        if (munmap(addr, 4096))
        {
            printf("munmap 8 failed\n");
            return -1;
        }
        // write st to the other 2 pages, to shows that they are still accessible
        size_t temp = (size_t) addr + 4096;
        for (int i = 0; i < (num_pages - 1); i++) {
            memcpy((char*) temp, st, strlen(st) + 1);
            temp += 4096;
        }
        temp = (size_t) addr + 4096;
        // munmap the other 2 pages
        if (munmap((void*) temp, (num_pages - 1) * 4096))
        {
            printf("munmap 8 failed\n");
            return -1;
        }
        exit(0);    // child should still exit successfully
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status != 0)
        {
            printf("Error fork 8: Child process did not finish successfully (code %d)\n", child_status);
            return -1;
        }
    }
    
    // read each page again to check if the write was successful
    size_t temp = (size_t) addr + 4096;
    for (int i = 1; i < num_pages; i++) {
        if (memcmp((char*) temp, st, strlen(st))) {
            printf("Error: reading/writing 8 failed\n");
            return -1;
        }
        temp += 4096;
    }
    // reset the file data and test munmap
    memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
    if (munmap(addr, num_pages * 4096))
    {
        printf("munmap 8 failed\n");
        return -1;
    }

    return 0;
}
