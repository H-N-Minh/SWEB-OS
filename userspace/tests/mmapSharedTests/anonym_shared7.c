#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

// TODO: currently close(fd) is never called

int anonym_shared7() {
    /////////////////////////////////////////////// TEST 7 ////////////////////////////////////////////////
    // Calling mmap and munmap with invalid parameters, these should all fail
    void* addr = 0;
    addr = mmap(NULL, 0, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);    // invalid length
    if (addr != MAP_FAILED) {
        printf("mmap 7.1 didnt failed as expected\n");
        return 1;
    }
    addr = 0;
    addr = mmap(NULL, 4096, 69, MAP_SHARED | MAP_ANONYMOUS, -1, 0);      // invalid prot
    if (addr != MAP_FAILED) {
        printf("mmap 7.2 didnt failed as expected\n");
        return 1;
    }
    addr = 0;
    addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, 0, -1, 0);      // invalid flags
    if (addr != MAP_FAILED) {
        printf("mmap 7.3 didnt failed as expected\n");
        return 1;
    }
    addr = 0;
    addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);      // invalid fd
    if (addr == MAP_FAILED) {
        printf("mmap 7.4 failed even though mmap should succeed with invalid fd\n");
        return 1;
    }
    if (!munmap(0, 4096))     // invalid addr
    {
        printf("munmap 7.5 didnt failed as expected\n");
        return -1;
    }
    if (!munmap(addr, 0))     // invalid length
    {
        printf("munmap 7.6 didnt failed as expected\n");
        return -1;
    }

    return 0;
}
