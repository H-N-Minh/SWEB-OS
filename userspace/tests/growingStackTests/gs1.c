#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#define PAGE_SIZE1 4096

size_t getTopOfThisPage(size_t address) 
{
  size_t top_stack = address - address%PAGE_SIZE1 + PAGE_SIZE1 - sizeof(size_t); 
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

// Test a very basic growing stack
int gs1()
{
    int i = 0;
    // printf("top of this page is %p (%zu)\n", (void*) getTopOfThisPage((size_t) &i), getTopOfThisPage((size_t) &i));
    int *p = &i;
    // printf("gs1: p = %p (%zu) with value %d (should be 0)\n", p, (size_t) p, *p);

    p =(int*) ((size_t)p - PAGE_SIZE1);
    // printf("now accessing new p at %p (%zu) \n", p, (size_t) p);
    *p = 11;
    printf("1 page added, currently 2 pages total\n");

    p =(int*) ((size_t)p - PAGE_SIZE1);
    *p = 22;
    printf("2 page added, currently 3 pages total\n");

    p =(int*) ((size_t)p - PAGE_SIZE1);
    *p = 33;
    printf("3 page added, currently 4 pages total\n");

    // p =(int*) ((size_t)p - PAGE_SIZE1);
    // *p = 44;
    // printf("4 page added, currently 5 pages total, this shouldnt be printed\n");
    return 0;
}
