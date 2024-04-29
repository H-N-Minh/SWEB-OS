#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int gs1();
extern int gs2();

/** TODO: 
 * - test calling pthread first, check if guards setup correctly, then call growing stack, then check guards again
 * - test growing stack first, check if guards setup correctly, then call pthread, then check guards again
 * - test with multiple threads, each with its own growing stack
 * - test with multiple threads, but a thread is trying to access another thread's unmapped stack (use &variable -= PAGE_SIZE*5)
 *                     
 * - test with fork ( fork then growing stack) and (growing stack then fork)
 * - test buffer over flow and underflow, program should exit with error code 
 * - test page is unmapped correctly when thread finish

*/

// set to 1 to test, 0 to skip
#define GS1 1     // basic test for growing stack
#define GS2 1     // threadStack.c

int main()
{
    int retval = 0;
    if (GS1)
    {
        printf("\nTesting gs1: basic test...\n");
        retval = gs1();
        if (retval == 0)                      { printf("===> gs1 successful!\n"); } 
        else                                  { printf("===> gs1 failed!\n");  return -1;}
    }

    if (GS2)
    {
      printf("\nTesting gs2.c ...\n");
      retval = gs2();
      if (retval == 0)                      { printf("===> gs2 successful!\n"); }
      else                                  { printf("===> gs2 failed!\n");  return -1;}
    }

    printf("\n\n---All tests completed! (press F12 to make sure all threads died correctly)---\n");
    return 0;
}