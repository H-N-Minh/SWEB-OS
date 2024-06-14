#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

int anonym_shared1() {
    //////////////////////////////////////////////// TEST 1 ////////////////////////////////////////////////
    // basic mmap, write and read and munmap. Parent should see the changes made by the child

    void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 1 failed\n");
        return 1;
    }
    // test reading
    if (memcmp(addr, "can u see me now", strlen("can u see me now") + 1))
    {
        printf("Error: reading 1 failed\n");
        return -1;
    }
    // test writing
    int *num = (int*) addr;
    *num = 10;
    assert(*num == 10 && "Error: writing 1 failed\n");
    // test IPC
    pid_t child_pid;
    int child_status;
    child_pid = fork();
    if (child_pid == 0) {
        assert(*num == 10 && "Error fork 1: after mmap then fork, Child sees different number \n");
        *num = 20;
        exit(0);
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status != 0)
        {
            printf("Error fork 1: Child process did not finish successfully (code %d)\n", child_status);
            return -1;
        }
        if (*num != 20) {
            printf("Error fork 1: Parent sees *num = %d, but it should be 20\n", *num);
            return -1;
        }
    }
    // reset the memory and test munmap
    memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
    if (munmap(addr, 4096))
    {
        printf("munmap 1 failed\n");
        return -1;
    }

    return 0;
}
