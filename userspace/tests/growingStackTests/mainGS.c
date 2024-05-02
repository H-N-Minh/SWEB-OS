#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"
#include "pthread.h"

extern int gs1();
extern int gs2();
extern int gs3();
extern int gs4();
extern int gs5();

/** TODO: 
 * - test calling pthread first, check if guards setup correctly, then call growing stack, then check guards again
 * - test growing stack first, check if guards setup correctly, then call pthread, then check guards again
 * - test with multiple threads, each with its own growing stack
 * - test with multiple threads, but a thread is trying to access another thread's unmapped stack (use &variable -= PAGE_SIZE*5)
 *                     
 * - test with fork ( fork then growing stack) and (growing stack then fork)
 * - test buffer over flow and underflow, program should exit with error code 
 * - test page is unmapped correctly when thread finish
// todo: test if all pages are freed

*/

// set to 1 to test, 0 to skip
#define GS1 0     // basic test for growing stack
#define GS2 0     // more advanced test for growing stack
#define GS3 0     // 100 threads created and grow its stack at the same time
#define GS4 0     // invalid growing 1: try to access outside stack limit
#define GS5 1     // invalid growing 2: try to access another thread's stack after that thread died

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
      printf("\nTesting gs3: 100 threads growing at same time ...\n");
      retval = gs3();
      if (retval == 0)                      { printf("===> gs3 successful!\n"); }
      else                                  { printf("===> gs3 failed!\n");  return -1;}
    }

    if (GS4)
    {
      printf("\nTesting gs4: invalid growing 1: try to access outside stack limit...\n");
      retval = gs4();
      if (retval == 0)                      { printf("===> gs4 successful!\n"); }
      else                                  { printf("===> gs4 failed!\n");  return -1;}
    }

    if (GS5)
    {
      printf("\nTesting gs5: invalid growing 2: try to access another thread's stack after that thread died...\n");
      retval = gs5();
      if (retval == 0)                      { printf("===> gs5 successful!\n"); }
      else                                  { printf("===> gs5 failed!\n");  return -1;}
    }

    printf("\n\n===   All tests completed!   ===\n");
    return 0;
  }
  else
  {
    int status;
    waitpid(cid, &status, 0);
    if (status != 0)
    {
      printf("\n\nXXX   Testing crashed with exit code %d   XXX\n", status);
    }
    
    for (size_t i = 0; i < 200000000; i++)      // give some time for all threads to die
    {
    /* code */
    }

    int num = get_thread_count();
    if (num == 7 || num == 6)
    {
      printf("===   All threads are destroyed correctly   ===\n");
      return 0;
    }
    else
    {
      printf("===   %d threads are still alive   ===\n", num);
      return -1;
    }
    
  }
  
}