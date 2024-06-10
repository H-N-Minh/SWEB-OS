#include <sys/mman.h> // for mmap
#include <stdio.h>    // for printf
#include <string.h>   // for strcpy


int mmap1() {
    
    // Size of the memory region
    size_t size = 4096;

    // Create a new memory region
    char *region = mmap(
        NULL,               // Let the system decide where to place the region
        size,               // Size of the region
        PROT_READ | PROT_WRITE, // The region is readable and writable
        MAP_ANONYMOUS | MAP_PRIVATE, // The region is not backed by any file
        -1,                 // File descriptor is not used
        0                   // Offset into the file
    );

    if (region == MAP_FAILED) {
        return -1;
    }
    printf("memory mapped at: %p\n", region);
    // Write to the memory region
    memcpy(region, "Hello, mmap1!", strlen("Hello, mmap!") + 1);
    printf("%s\n", region);

    // // Unmap the memory region
    // if (munmap(region, 4096))
    // {
    //     printf("munmap didnt return with code 0\n");
    //     return -1;
    // }
    // printf("memory unmapped1\n");
    // if (munmap(region, 12))
    // {
    //     printf("munmap didnt return with code 0\n");
    //     return -1;
    // }
    // printf("memory unmapped2\n");
    

    return 0;
}