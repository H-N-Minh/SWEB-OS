#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"
#include "pthread.h"

extern int gs1();
extern int gs2();
extern int gs3();

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
#define GS2 1     // more advanced test for growing stack
#define GS3 0     // (not complete) test with multiple threads, each will grow its stack at the same time
#define GS4 0     // (not complete) test invalid growing stack (kill another thread then try to access it)

int main()
{
  pid_t cid = fork();
  if (cid == 0)
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
      printf("\nTesting gs2: more complicated test ...\n");
      retval = gs2();
      if (retval == 0)                      { printf("===> gs2 successful!\n"); }
      else                                  { printf("===> gs2 failed!\n");  return -1;}
    }

    if (GS3)
    {
      printf("\nTesting gs3: multiple threads growing at same time ...\n");
      retval = gs3();
      if (retval == 0)                      { printf("===> gs3 successful!\n"); }
      else                                  { printf("===> gs3 failed!\n");  return -1;}
    }

    printf("\n\n---All tests completed!\n");
    return 0;
  }
  else
  {
    int status;
    waitpid(cid, &status, 0);
    if (status != 0)
    {
      printf("Testing crashed with exit code %d\n", status);
    }
    
    for (size_t i = 0; i < 200000000; i++)      // give some time for all threads to die
    {
    /* code */
    }

    int num = get_thread_count();
    if (num == 7 || num == 6)
    {
      printf("===  All threads are destroyed correctly\n   ===");
      return 0;
    }
    else
    {
      printf("===  %d threads are still alive\n   ===", num);
      return -1;
    }
    
  }
  
}