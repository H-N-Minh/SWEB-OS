/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/


#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define PAGE_SIZE1 4096


// Test a very basic growing stack
int main()
{
    int i = 0;
    // printf("top of this page is %p (%zu)\n", (void*) getTopOfThisPage((size_t) &i), getTopOfThisPage((size_t) &i));
    int *p = &i;
    // printf("gs1: p = %p (%zu) with value %d (should be 0)\n", p, (size_t) p, *p);

    p =(int*) ((size_t)p - PAGE_SIZE1);
    // printf("now accessing new p at %p (%zu) \n", p, (size_t) p);
    *p = 11;
    assert(*p == 11 && "gs1: p = 11 failed");

    p =(int*) ((size_t)p - PAGE_SIZE1);
    *p = 22;
    assert(*p == 22 && "gs1: p = 22 failed");

    p =(int*) ((size_t)p - PAGE_SIZE1);
    *p = 33;
    assert(*p == 33 && "gs1: p = 33 failed");

    // p =(int*) ((size_t)p - PAGE_SIZE1);
    // *p = 44;
    // printf("4 page added, currently 5 pages total, this shouldnt be printed\n");
    return 0;
}
