#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

// TODO: currently close(fd) is never called
int shared1() {
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    //////////////////////////////////////////////// TEST 1 ////////////////////////////////////////////////
    // basic mmap, write and read and munmap. Parent should see the changes made by the child

    void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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
    // reset the file data and test munmap
    memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
    if (munmap(addr, 4096))
    {
        printf("munmap 1 failed\n");
        return -1;
    }

    //////////////////////////////////////////////// TEST 2 ////////////////////////////////////////////////
    // mmap with read-only protection, child tries to write, parent expects child to crash
    addr = MAP_FAILED;
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
    child_pid = -1;
    child_status = -1;
    child_pid = fork();
    if (child_pid == 0) {
        assert(*num == 10 && "Error fork 2: after mmap then fork, Child sees different number \n");
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

    /////////////////////////////////////////////// TEST 3 ////////////////////////////////////////////////
    // mmap with larger size, child writes to every page, Parent should see all the changes made by the child
    addr = MAP_FAILED;
    int pages = 20;
    addr = mmap(NULL, pages * 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 3 failed\n");
        return 1;
    }
    // test IPC
    child_pid = -1;
    child_status = -1;
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
