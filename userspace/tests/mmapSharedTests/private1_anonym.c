#include <sys/mman.h> // for mmap
#include <stdio.h>    // for printf
#include <string.h>   // for strcpy
#include <unistd.h>
#include <fcntl.h>    // for open

// same as private1, but uses MAP_ANONYMOUS instead of a file descriptor.
// for test details, see private1.c
int private1_anonym() {
    
    //////////////////////////////////////////////// TEST 1 ////////////////////////////////////////////////
    // basic mmap, write and read and munmap

    char *region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE | MAP_ANONYMOUS, // The region is not backed by any file
        -1,                 // File descriptor is not used
        0                   // Offset into the file
    );

    if (region == MAP_FAILED) {
        printf("mmap 1 failed\n");
        return -1;
    }

    memcpy(region, "Hello, mmap1!", strlen("Hello, mmap1!") + 1);       // simulates write operation
    if (memcmp(region, "Hello, mmap1!", strlen("Hello, mmap1!") + 1))   // simulates read operation
    {
        printf("Error: writing 1 failed\n");
        return -1;
    }
    if (munmap(region, 4096))
    {
        printf("munmap 1 failed\n");
        return -1;
    }


    //////////////////////////////////////////////// TEST 2 ////////////////////////////////////////////////
    // different size of mmap
    int pages = 20;
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        (4096 * pages),               // Size of the region
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE | MAP_ANONYMOUS, // The region is not backed by any file
        -1,                 // File descriptor is not used
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
    
    //////////////////////////////////////////////// TEST 3 ////////////////////////////////////////////////
    // readonly prot
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region
        PROT_READ, 
        MAP_PRIVATE | MAP_ANONYMOUS, 
        -1,                 // File descriptor is not used
        0                   // Offset into the file
    );

    if (region == MAP_FAILED) {
        printf("mmap 3 failed\n");
        return -1;
    }

    if (memcmp(region, "", 1))     // this should pass, since map_anonym create empty page
    {
        printf("Error: reading 3 failed\n");
        return -1;
    }
    if (!memcmp(region, " ", 2))     // this should fail
    {
        printf("Error: reading 3 failed\n");
        return -1;
    }
    if (munmap(region, 4096))
    {
        printf("munmap 3 failed\n");
        return -1;
    }

    //////////////////////////////////////////////// TEST 4 ////////////////////////////////////////////////
    // writeonly prot
    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region
        PROT_WRITE, 
        MAP_PRIVATE | MAP_ANONYMOUS, 
        -1,                 // File descriptor is not used
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

    //////////////////////////////////////////// TEST 5 ////////////////////////////////////////////
    // mmap 3 pages, but then munmap partially: first 1 page (the other 2 should be accessible), then the other 2 pages at the same time

    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        (4096 * 3),               // Size of the region (2 pages)
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE | MAP_ANONYMOUS, // The region is not backed by any file
        -1,                 // File descriptor is not used
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

    //////////////////////////////////////////// TEST 6 ////////////////////////////////////////////
    // mmap 1 page, but then munmap 2 pages, then munmap with size that is not page aligned, then munmap same page twice

    region = MAP_FAILED;
    region = mmap(
        NULL,               // Let the system decide where to place the region
        4096,               // Size of the region (1 page)
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_PRIVATE | MAP_ANONYMOUS, // The region is not backed by any file
        -1,                 // File descriptor is not used
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
    if (!munmap(region, 4097))       // unmap more than mapped, this should fail
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

    return 0;
}