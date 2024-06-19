#include <sys/mman.h> // for mmap
#include <stdio.h>    // for printf
#include <string.h>   // for strcpy
#include <unistd.h>
#include <fcntl.h>    // for open

// basic test.
// Test 1: very basic mmap, write and read and munmap
// Test 2: same test but with 20 pages as size
// Test 3: use only read-only prot
// Test 4: use only write-only prot
// Test 5: test munmap: mmap 3 pages, but then munmap partially: first 1 page (the other 2 should be accessible), then the other 2 pages at the same time
// Test 6: test munmap: mmap 1 page, but then munmap 2 pages (should fail), then munmap with size that is not page aligned (should work), then munmap same page twice (should fail)
int private1() {
    
    //////////////////////////////////////////////// TEST 1 ////////////////////////////////////////////////
    // basic mmap, write and read and munmap
    int fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open 1 failed\n");
        return 1;
    }
    char *region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE, // The region is not backed by any file
        fd,                 // File descriptor is not used
        0                   // Offset into the file
    );

    if (region == MAP_FAILED) {
        printf("mmap 1 failed\n");
        return -1;
    }

    memcpy(region, "Hello, mmap1!", strlen("Hello, mmap1!") + 1);
    if (memcmp(region, "Hello, mmap1!", strlen("Hello, mmap1!") + 1))
    {
        printf("Error: writing 1 failed\n");
        return -1;
    }
    if (munmap(region, 4096))
    {
        printf("munmap 1 failed\n");
        return -1;
    }
    close(fd);


    //////////////////////////////////////////////// TEST 2 ////////////////////////////////////////////////
    // different size of mmap
    int pages = 20;
    fd = -1;
    fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open 2 failed\n");
        return 1;
    }
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        (4096 * pages),               // Size of the region
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE, // The region is not backed by any file
        fd,                 // File descriptor is not used
        0                   // Offset into the file
    );

    if (region == MAP_FAILED) {
        printf("mmap 2 failed\n");
        return -1;
    }
    char* region2 = region;
    for (size_t i = 0; i < pages; i++)
    {
        memcpy(region2, "Hello, mmap2!", strlen("Hello, mmap2!") + 1);
        region2 = (char*)((size_t)region2 + 4096);
    }
    region2 = region;
    for (size_t i = 0; i < pages; i++)
    {
        if (memcmp(region2, "Hello, mmap2!", strlen("Hello, mmap2!") + 1))
        {
            printf("Error: writing 2 failed\n");
            return -1;
        }
        region2 = (char*)((size_t)region2 + 4096);
    }
    if (munmap(region, 4096 * pages))
    {
        printf("munmap 2 failed\n");
        return -1;
    }
    close(fd);
    
    //////////////////////////////////////////////// TEST 3 ////////////////////////////////////////////////
    // readonly prot
    fd = -1;
    fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open 3 failed\n");
        return -1;
    }
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region
        PROT_READ, 
        MAP_PRIVATE, 
        fd,                 // File descriptor is not used
        0                   // Offset into the file
    );

    if (region == MAP_FAILED) {
        printf("mmap 3 failed\n");
        return -1;
    }

    if (memcmp(region, "can u see me now", strlen("can u see me now") + 1))     // this should pass
    {
        printf("Error: reading 3 failed\n");
        return -1;
    }
    if (!memcmp(region, "can U see me now", strlen("can U see me now") + 1))    // this should fail
    {
        printf("Error: reading 3 failed\n");
        return -1;
    }
    if (munmap(region, 4096))
    {
        printf("munmap 3 failed\n");
        return -1;
    }
    close(fd);

    //////////////////////////////////////////////// TEST 4 ////////////////////////////////////////////////
    // writeonly prot
    fd = -1;
    fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open 4 failed\n");
        return -1;
    }
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region
        PROT_WRITE, 
        MAP_PRIVATE, 
        fd,                 // File descriptor is not used
        0                   // Offset into the file
    );

    if (region == MAP_FAILED) {
        printf("mmap 4 failed\n");
        return -1;
    }
    memcpy(region, "Hello, mmap4!", strlen("Hello, mmap4!") + 1);
    if (munmap(region, 4096))
    {
        printf("munmap 4 failed\n");
        return -1;
    }
    close(fd);

    //////////////////////////////////////////// TEST 5 ////////////////////////////////////////////
    // mmap 3 pages, but then munmap partially: first munmap 1 page (the other 2 should be accessible), then munmap the other 2 pages at the same time
    fd = -1;
    fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open 5 failed\n");
        return -1;
    }
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        (4096 * 3),               // Size of the region (2 pages)
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE, // The region is not backed by any file
        fd,                 // File descriptor is not used
        0                   // Offset into the file
    );
    if (region == MAP_FAILED) {
        printf("mmap 5 failed\n");
        return -1;
    }
    char* region5 = region;
    for (size_t i = 0; i < 3; i++)
    {
        memcpy(region5, "Hello, mmap5!", strlen("Hello, mmap5!") + 1);
        region5 = (char*)((size_t)region5 + 4096);
    }
    region5 = region;
    for (size_t i = 0; i < 3; i++)
    {
        if (memcmp(region5, "Hello, mmap5!", strlen("Hello, mmap5!") + 1))
        {
            printf("Error: writing 5 failed\n");
            return -1;
        }
        region5 = (char*)((size_t)region5 + 4096);
    }
    
    if (munmap(region, 4096))       // unmap 1 page, the other 2 pages should still be accessible
    {
        printf("munmap 5 failed\n");
        return -1;
    }
    region = (char*)((size_t)region + 4096);
    region5 = region;
    for (size_t i = 0; i < 2; i++)
    {
        memcpy(region5, "Hello, mmap5!", strlen("Hello, mmap5!") + 1);
        region5 = (char*)((size_t)region5 + 4096);
    }
    region5 = region;
    for (size_t i = 0; i < 2; i++)
    {
        if (memcmp(region5, "Hello, mmap5!", strlen("Hello, mmap5!") + 1))
        {
            printf("Error: writing 5 failed\n");
            return -1;
        }
        region5 = (char*)((size_t)region5 + 4096);
    }
    if (munmap(region, (4096 * 2)))   // munmap the other 2 pages, this should pass
    {
        printf("munmap 5 failed\n");
        return -1;
    }
    close(fd);

    //////////////////////////////////////////// TEST 6 ////////////////////////////////////////////
    // mmap 1 page, but then munmap 2 pages, then munmap with size that is not page aligned, then munmap same page twice
    fd = -1;
    fd = open("usr/testmmn.txt", O_RDWR | O_CREAT , 0644);
    if (fd == -1) {
        printf("open 6 failed\n");
        return -1;
    }
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region (1 page)
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE, // The region is not backed by any file
        fd,                 // File descriptor is not used
        0                   // Offset into the file
    );
    if (region == MAP_FAILED) {
        printf("mmap 6 failed\n");
        return -1;
    }
    memcpy(region, "Hello, mmap6!", strlen("Hello, mmap6!") + 1);
    if (memcmp(region, "Hello, mmap6!", strlen("Hello, mmap6!") + 1))     // this should pass
    {
        printf("Error: reading 6 failed\n");
        return -1;
    }
    if (!munmap(region, 4097))       // unmap 2 pages, this should fail
    {
        printf("munmap 6 failed\n");
        return -1;
    }
    if (munmap(region, 1))       // munmap less than 1 page, this should be rounded up to 1 page, page should still be unmapped
    {
        printf("munmap 6 failed\n");
        return -1;
    }
    if (!munmap(region, 1))       // unmap the same page again, this should fail
    {
        printf("munmap 6 failed\n");
        return -1;
    }
    close(fd);

    return 0;
}