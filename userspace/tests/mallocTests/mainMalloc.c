#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int sbrk1();
extern int brk1();


// set to 1 to test, 0 to skip
// TODO: ask if we would get point for implementing brk if we already have sbrk
#define SBRK1 1     // basic test for sbrk
#define BRK1 1      // basic test for brk

int main()
{
    int retval = 0;
    if (SBRK1)
    {
        printf("\nTesting sbrk1: basic test...\n");
        retval = sbrk1();
        if (retval == 0)                      { printf("===> sbrk1 successful!\n"); } 
        else                                  { printf("===> sbrk1 failed!\n");  return -1;}
    }

    if (BRK1)
    {
        printf("\nTesting brk1: basic test...\n");
        retval = brk1();
        if (retval == 0)                      { printf("===> brk1 successful!\n"); } 
        else                                  { printf("===> brk1 failed!\n");  return -1;}
    }

    printf("\n\n---All tests completed! (press F12 to make sure all threads died correctly)---\n");
    return 0;
}