#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define PAGE_SIZE1 4096

// Test a very basic growing stack
int gs1()
{
    int i = 0;
    int *p = &i;
    printf("gs1: p = %p (%zu) with value %d (should be 0)\n", p, (size_t) p, *p);
    // p = (int*) ((size_t) p + PAGE_SIZE1);
    p -= PAGE_SIZE1;
    printf("now accessing new p at %p (%zu) \n", p, (size_t) p);
    *p = 11;
    printf("gs1: p = %p with value %d (should be 11)\n", p, *p);
    

    return 0;
}
