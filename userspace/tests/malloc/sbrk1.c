#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"



// Test a very basic fork
int sbrk1()
{
    // Allocate 100 bytes using sbrk
    void* ptr1 = sbrk(100);
    assert(ptr1 != (void*)-1);

    // Allocate 200 bytes using sbrk
    void* ptr2 = sbrk(200);
    assert(ptr2 != (void*)-1);

    // Deallocate 100 bytes using sbrk
    void* ptr3 = sbrk(-100);
    assert(ptr3 != (void*)-1);

    // Deallocate 200 bytes using sbrk
    void* ptr4 = sbrk(-200);
    assert(ptr4 != (void*)-1);

    return 0;
}