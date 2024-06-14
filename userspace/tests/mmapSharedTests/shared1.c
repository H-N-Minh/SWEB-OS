#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

// TODO: currently close(fd) is never called
// Test 1: very basic mmap, write and read and munmap, and IPC between 2 processes
// Test 2: mmap with read-only protection, child tries to write and should crash
// Test 3: mmap with larger size, child writes to every page, Parent should see all the changes made by the child
// Test 4: Child calls munmap, parent should still be able to access the page and then call munmap itself on the same page
// Test 5: Child calls munmap with too big size (should fail), then with small size (size should be rounded up and page should still be unmapped), 
//          then munmap same page again (should fail). Child shouldnt crash in this test
// Test 6: Child calls munmap, then tries to write to page, child should crashes.
// Test 7: Calling mmap and munmap with invalid parameters, these should all fail
// Test 8: Child munmaps the first page, writes to the other 2 pages (to show that they are still accessible), 
//          then munmaps those 2 pages at the same time. child shouldnt crash
// Test 9: Child writes to the shared page, then exits without munmap. Parent should still be able to see what the child wrote. 
//          This shows the page is written back to file (even without munmap) when process terminates
int shared1() {
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open failed\n");
        return 1;
    }

    //////////////////////////////////////////////// TEST 1 ////////////////////////////////////////////////
    // basic mmap, write and read and munmap. Parent should see the changes made by the child

    // void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // if (addr == MAP_FAILED) {
    //     printf("mmap 1 failed\n");
    //     return 1;
    // }
    // // test reading
    // if (memcmp(addr, "can u see me now", strlen("can u see me now") + 1))
    // {
    //     printf("Error: reading 1 failed\n");
    //     return -1;
    // }
    // // test writing
    // int *num = (int*) addr;
    // // *num = 10;
    // // assert(*num == 10 && "Error: writing 1 failed\n");
    // // test IPC
    // pid_t child_pid;
    // int child_status;
    // child_pid = fork();
    // if (child_pid == 0) {
    //     assert(*num == 10 && "Error fork 1: after mmap then fork, Child sees different number \n");
    //     *num = 20;
    //     exit(0);
    // } 
    // else 
    // {
    //     waitpid(child_pid, &child_status, 0);
    //     if (child_status != 0)
    //     {
    //         printf("Error fork 1: Child process did not finish successfully (code %d)\n", child_status);
    //         return -1;
    //     }
    //     if (*num != 20) {
    //         printf("Error fork 1: Parent sees *num = %d, but it should be 20\n", *num);
    //         return -1;
    //     }
    // }
    // // reset the file data and test munmap
    // memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
    // if (munmap(addr, 4096))
    // {
    //     printf("munmap 1 failed\n");
    //     return -1;
    // }

//     //////////////////////////////////////////////// TEST 2 ////////////////////////////////////////////////
//     // mmap with read-only protection, child tries to write, parent expects child to crash
//     addr = MAP_FAILED;
//     addr = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         printf("mmap 2 failed\n");
//         return 1;
//     }
//     // test reading
//     if (memcmp(addr, "can u see me now", strlen("can u see me now") + 1))
//     {
//         printf("Error: reading 2 failed\n");
//         return -1;
//     }
//     // test IPC
//     child_pid = -1;
//     child_status = -1;
//     child_pid = fork();
//     if (child_pid == 0) {
//         assert(*num == 10 && "Error fork 2: after mmap then fork, Child sees different number \n");
//         *num = 20;
//         exit(0);
//     } 
//     else 
//     {
//         waitpid(child_pid, &child_status, 0);
//         if (child_status == 0)
//         {
//             printf("Error fork 2: Child process did not crash as expected (code %d)\n", child_status);
//             return -1;
//         }
//     }
//     // reset the file data and test munmap
//     if (munmap(addr, 4096))
//     {
//         printf("munmap 2 failed\n");
//         return -1;
//     }

    /////////////////////////////////////////////// TEST 3 ////////////////////////////////////////////////
    // mmap with larger size, child writes to every page, Parent should see all the changes made by the child
    void* addr = MAP_FAILED;
    int pages = 1;
    addr = mmap(NULL, pages * 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        printf("mmap 3 failed\n");
        return 1;
    }
    printf("mmap 3 done\n");
    // test IPC
    pid_t child_pid = -1;
    int child_status = -1;
    char *st = "test 3";
    child_pid = fork();
    if (child_pid == 0) {
        // write st to every page
        printf("child writing to the page\n");
        size_t temp = (size_t) addr;
        for (int i = 0; i < pages; i++) {
            memcpy((char*) temp, st, strlen(st) + 1);
            temp += 4096;
        }
        printf("child wrote to the page\n");
        exit(0);
    } 
    else 
    {
        waitpid(child_pid, &child_status, 0);
        printf("parent reading the page\n");
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
        printf("parent done read the page\n");
    }
    // reset the file data and test munmap
    memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
    if (munmap(addr, pages * 4096))
    {
        printf("munmap 3 failed\n");
        return -1;
    }

//     /////////////////////////////////////////////// TEST 4 ////////////////////////////////////////////////
//     // Child calls munmap, parent should still be able to access the page and then call munmap itself on the same page
//     addr = MAP_FAILED;
//     addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         printf("mmap 4 failed\n");
//         return 1;
//     }
//     // test IPC
//     child_pid = -1;
//     child_status = -1;
//     st = "test 4";
//     child_pid = fork();
//     if (child_pid == 0) {
//         // write st to the page
//         memcpy(addr, st, strlen(st) + 1);
//         if (munmap(addr, 4096))
//         {
//             printf("munmap 4 failed\n");
//             return -1;
//         }
//         exit(0);
//     } 
//     else 
//     {
//         waitpid(child_pid, &child_status, 0);
//         if (child_status != 0)
//         {
//             printf("Error fork 4: Child process did not finish successfully (code %d)\n", child_status);
//             return -1;
//         }
//         // read the page to check if the write was successful
//         if (memcmp(addr, st, strlen(st))) {
//             printf("Error: reading/writing 4 failed\n");
//             return -1;
//         }
//         if (munmap(addr, 4096))
//         {
//             printf("munmap 4 failed\n");
//             return -1;
//         }
//     }

//     /////////////////////////////////////////////// TEST 5 ////////////////////////////////////////////////
//     // Child calls munmap with too big size (should fail), then with small size (size should be rounded up and page should still be unmapped), then munmap again (should fail). Child shouldnt crash in this test
//     addr = MAP_FAILED;
//     addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         printf("mmap 5 failed\n");
//         return 1;
//     }
//     // test IPC
//     child_pid = -1;
//     child_status = -1;
//     st = "test 4";
//     child_pid = fork();
//     if (child_pid == 0) {
//         if (!munmap(addr, 4097)) // too big size, this would be rounded up to 2 pages, which causes munmap to fail
//         {
//             printf("munmap 5 didnt fail as expected\n");
//             return -1;
//         }
//         if (munmap(addr, 1)) // munmap less than 1 page, this should be rounded up to 1 page, page should still be unmapped
//         {
//             printf("munmap 5 failed\n");
//             return -1;
//         }
//         if (!munmap(addr, 1)) // unmap the same page again, this should fail
//         {
//             printf("munmap 5 didnt fail as expected\n");
//             return -1;
//         }
//         exit(0);    // child should still exit successfully
//     } 
//     else 
//     {
//         waitpid(child_pid, &child_status, 0);
//         if (child_status != 0)
//         {
//             printf("Error fork 5: Child process did not finish successfully (code %d)\n", child_status);
//             return -1;
//         }
//         if (munmap(addr, 4096))
//         {
//             printf("munmap 5 failed\n");
//             return -1;
//         }
//     }

//     /////////////////////////////////////////////// TEST 6 ////////////////////////////////////////////////
//     // Child calls munmap, then tries to write to page, child should crashes.
//     addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         printf("mmap 6 failed\n");
//         return 1;
//     }
//     // test IPC
//     child_pid = -1;
//     child_status = -1;
//     st = "test 6";
//     child_pid = fork();
//     if (child_pid == 0) {
//         // write st to the page
//         if (munmap(addr, 4096))
//         {
//             printf("munmap 6 failed\n");
//             return -1;
//         }
//         // try to write to the page after munmap
//         memcpy(addr, st, strlen(st) + 1);
//         exit(0);
//     } 
//     else 
//     {
//         waitpid(child_pid, &child_status, 0);
//         if (child_status == 0)
//         {
//             printf("Error fork 6: Child process did not crash as expected (code %d)\n", child_status);
//             return -1;
//         }
//         if (munmap(addr, 4096))
//         {
//             printf("munmap 6 failed\n");
//             return -1;
//         }
//     }

//     /////////////////////////////////////////////// TEST 7 ////////////////////////////////////////////////
//     // Calling mmap and munmap with invalid parameters, these should all fail
//     addr = 0;
//     addr = mmap(NULL, 0, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);    // invalid length
//     if (addr != MAP_FAILED) {
//         printf("mmap 7.1 didnt failed as expected\n");
//         return 1;
//     }
//     addr = 0;
//     addr = mmap(NULL, 4096, 69, MAP_SHARED, fd, 0);      // invalid prot
//     if (addr != MAP_FAILED) {
//         printf("mmap 7.2 didnt failed as expected\n");
//         return 1;
//     }
//     addr = 0;
//     addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, 0, fd, 0);      // invalid flags
//     if (addr != MAP_FAILED) {
//         printf("mmap 7.3 didnt failed as expected\n");
//         return 1;
//     }
//     addr = 0;
//     addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);      // invalid fd
//     if (addr != MAP_FAILED) {
//         printf("mmap 7.4 didnt failed as expected\n");
//         return 1;
//     }
//     if (!munmap(0, 4096))     // invalid addr
//     {
//         printf("munmap 7 didnt failed as expected\n");
//         return -1;
//     }
//     if (!munmap(addr, 0))     // invalid length
//     {
//         printf("munmap 7 didnt failed as expected\n");
//         return -1;
//     }

//     // /////////////////////////////////////////////// TEST 8 ////////////////////////////////////////////////
//     // Child munmaps the first page, writes to the other 2 pages, then munmaps the other 2 pages. child should exit successfully
//     addr = MAP_FAILED;
//     int num_pages = 3;
//     addr = mmap(NULL, num_pages * 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         printf("mmap 8 failed\n");
//         return 1;
//     }
//     // test IPC
//     child_pid = -1;
//     child_status = -1;
//     st = "test 8";
//     child_pid = fork();
//     if (child_pid == 0) {
//         // munmap the first page
//         if (munmap(addr, 4096))
//         {
//             printf("munmap 8 failed\n");
//             return -1;
//         }
//         // write st to the other 2 pages, to shows that they are still accessible
//         size_t temp = (size_t) addr + 4096;
//         for (int i = 1; i < (num_pages - 1); i++) {
//             memcpy((char*) temp, st, strlen(st) + 1);
//             temp += 4096;
//         }
//         temp = (size_t) addr + 4096;
//         // munmap the other 2 pages
//         if (munmap((void*) temp, (num_pages - 1) * 4096))
//         {
//             printf("munmap 8 failed\n");
//             return -1;
//         }
//         exit(0);    // child should still exit successfully
//     } 
//     else 
//     {
//         waitpid(child_pid, &child_status, 0);
//         if (child_status != 0)
//         {
//             printf("Error fork 8: Child process did not finish successfully (code %d)\n", child_status);
//             return -1;
//         }
//     }
//     // reset the file data and test munmap
//     memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
//     if (munmap(addr, num_pages * 4096))
//     {
//         printf("munmap 8 failed\n");
//         return -1;
//     }

//     /////////////////////////////////////////////// TEST 9 ////////////////////////////////////////////////
//     // Child writes to the shared page, then exits without munmap. Parent should still be able to see what the child wrote. This shows the page is written back to file (even without munmap) when process terminates
//     addr = MAP_FAILED;
//     addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//     if (addr == MAP_FAILED) {
//         printf("mmap 9 failed\n");
//         return 1;
//     }
//     child_pid = -1;
//     child_status = -1;
//     st = "test 9";
//     child_pid = fork();
//     if (child_pid == 0) {
//         // write st to the page, then exits without mmunmap
//         memcpy(addr, st, strlen(st) + 1);
//         exit(0);
//     } 
//     else 
//     {
//         waitpid(child_pid, &child_status, 0);
//         if (child_status != 0)
//         {
//             printf("Error fork 9: Child process did not finish successfully (code %d)\n", child_status);
//             return -1;
//         }
//         // check if the parent can see what the child wrote
//         if (memcmp(addr, st, strlen(st))) {
//             printf("Error fork 9: the data the child wrote should be written onto the file after child died\n");
//             return -1;
//         }
//     }
//     // reset the file data and test munmap
//     memcpy(addr, "can u see me now", strlen("can u see me now") + 1);
//     if (munmap(addr, 4096))
//     {
//         printf("munmap 8 failed\n");
//         return -1;
//     }

    return 0;
}
