#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"



// Test a very basic fork
int sbrk1()
{
    // Allocate 100 bytes using sbrk
    void* ptr1 = sbrk(0);
    assert(ptr1 != (void*)-1);
    printf("ptr1: %p\n", ptr1);

    // Allocate 200 bytes using sbrk
    void* ptr2 = sbrk(0);
    assert(ptr2 != (void*)-1);
    printf("ptr2: %p\n", ptr1);

    // Deallocate 100 bytes using sbrk
    void* ptr3 = sbrk(-0);
    assert(ptr3 != (void*)-1);
    printf("ptr3: %p\n", ptr1);

    // Deallocate 200 bytes using sbrk
    void* ptr4 = sbrk(-0);
    assert(ptr4 != (void*)-1);
    printf("ptr4: %p\n", ptr1);

    return 0;
}