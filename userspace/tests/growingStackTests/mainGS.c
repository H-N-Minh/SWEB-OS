#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int gs1();


// set to 1 to test, 0 to skip
#define GS1 1     // basic test for growing stack

int main()
{
    int retval = 0;
    if (GS1)
    {
        printf("\nTesting growing_stack_1: basic test...\n");
        retval = gs1();
        if (retval == 0)                      { printf("===> gs1 successful!\n"); } 
        else                                  { printf("===> gs1 failed!\n");  return -1;}
    }

    printf("\n\n---All tests completed! (press F12 to make sure all threads died correctly)---\n");
    return 0;
}