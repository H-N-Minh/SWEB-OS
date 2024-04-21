#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"

// Test a very basic sbrk
int sbrk1()
{
    void* ptr0 = sbrk(0);
    assert(ptr0 != (void*)-1);
    printf("break is at: %p ( %lu)\n", ptr0, (size_t)ptr0);

    // Deallocate 100 bytes using sbrk
    printf("Deallocating 100 bytes\n");
    void* ptr1 = sbrk(-100);
    assert(ptr1 == (void*)-1);
    void* ptr11 = sbrk(0);
    assert(ptr11 != (void*)-1);
    printf("break is at: %p ( %lu)\n", ptr11, (size_t)ptr11);
    assert(ptr0 == ptr11);

    // Allocate 200 bytes using sbrk
    printf("Allocating 200 bytes\n");
    void* ptr2 = sbrk(200);
    assert(ptr2 != (void*)-1);
    void* ptr22 = sbrk(0);
    assert(ptr22 != (void*)-1);
    printf("break is at: %p ( %lu)\n", ptr22, (size_t)ptr22);
    assert((size_t)ptr0 + 200 == (size_t)ptr22);

    // Deallocate 201 bytes using sbrk
    printf("Deallocating 201 bytes\n");
    void* ptr3 = sbrk(-201);
    assert(ptr3 == (void*)-1);
    void* ptr33 = sbrk(0);
    assert(ptr33 != (void*)-1);
    printf("break is at: %p ( %lu)\n", ptr33, (size_t)ptr33);
    assert(ptr22 == ptr33);

    // Deallocate 199 bytes using sbrk
    printf("Deallocating 199 bytes\n");
    void* ptr4 = sbrk(-199);
    assert(ptr4 != (void*)-1);
    void* ptr44 = sbrk(0);
    assert(ptr44 != (void*)-1);
    printf("break is at: %p ( %lu)\n", ptr44, (size_t)ptr44);
    assert((size_t)ptr44 == (size_t)ptr33 - 199);

    // Deallocate 1 bytes using sbrk
    printf("Deallocating 1 bytes\n");
    void* ptr5 = sbrk(-1);
    assert(ptr5 != (void*)-1);
    void* ptr55 = sbrk(0);
    assert(ptr55 != (void*)-1);
    printf("break is at: %p ( %lu)\n", ptr55, (size_t)ptr55);
    assert((size_t)ptr55 == (size_t)ptr44 - 1);

    assert(ptr0 == ptr55);

    return 0;
}
