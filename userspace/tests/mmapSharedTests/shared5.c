#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

// TODO: currently close(fd) is never called

int shared5() {
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    /////////////////////////////////////////////// TEST 5 ////////////////////////////////////////////////
    // Child calls munmap with too big size (should fail), then with small size (size should be rounded up and page should still be unmapped), then munmap again (should fail). Child shouldnt crash in this test
    void* addr = MAP_FAILED;
    addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 5 failed\n");
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
    char *st = "test 4";
    child_pid = fork();
    if (child_pid == 0) {
        if (!munmap(addr, 4097)) // too big size, this would be rounded up to 2 pages, which causes munmap to fail
        {
            printf("munmap 5 didnt fail as expected\n");
            return -1;
        }
        if (munmap(addr, 1)) // munmap less than 1 page, this should be rounded up to 1 page, page should still be unmapped
        {
            printf("munmap 5 failed\n");
            return -1;
        }
        if (!munmap(addr, 1)) // unmap the same page again, this should fail
        {
            printf("munmap 5 didnt fail as expected\n");
            return -1;
        }
        exit(0);    // child should still exit successfully
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        if (child_status != 0)
        {
            printf("Error fork 5: Child process did not finish successfully (code %d)\n", child_status);
            return -1;
        }
        if (munmap(addr, 4096))
        {
            printf("munmap 5 failed\n");
            return -1;
        }
    }

    return 0;
}
