#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

int anonym_shared3() {
    /////////////////////////////////////////////// TEST 3 ////////////////////////////////////////////////
    // mmap with larger size, child writes to every page, Parent should see all the changes made by the child
    void* addr = MAP_FAILED;
    int pages = 20;
    addr = mmap(NULL, pages * 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 3 failed\n");
        return 1;
    }
    
    // test IPC
    pid_t child_pid = -1;
    int child_status = -1;
    char *st = "test 3";
    child_pid = fork();
    if (child_pid == 0) {
        // write st to every page
        size_t temp = (size_t) addr;
        for (int i = 0; i < pages; i++) {
            memcpy((char*) temp, st, strlen(st) + 1);
            temp += 4096;
        }
        exit(0);
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status != 0)
        {
            printf("Error fork 3: Child process did not finish successfully (code %d)\n", child_status);
            return -1;
        }
        size_t temp = (size_t) addr;
        // read each page again to check if the write was successful
        for (int i = 0; i < pages; i++) {
            if (memcmp((char*) temp, st, strlen(st))) {
                printf("Error: reading/writing 3 failed\n");
                return -1;
            }
            temp += 4096;
        }
    }
    // reset the file data and test munmap
    memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
    if (munmap(addr, pages * 4096))
    {
        printf("munmap 3 failed\n");
        return -1;
    }

    return 0;
}
