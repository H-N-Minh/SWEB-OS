#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int sem1();
extern int sem2();
extern int sem3();

extern int sem7();
extern int sem9();

// set to 1 to test, 0 to skip
#define SEM1 1                // simple test
#define SEM2 2                // cond and sem together
#define SEM3 3                // multiple threads waiting on same sem, also test exiting while threads still sleeping

#define SEM7 7
#define SEM9 9  //test with sem init > 1


int main()
{
    int retval = 0;

    if (SEM1)
    {
        printf("\nTesting sem1: simple test...\n");
        retval = sem1();
        if (retval == 0)                      { printf("===> sem1 successful!\n"); } 
        else                                  { printf("===> sem1 failed!\n");  return -1;}
    }

    if (SEM2)
    {
        printf("\nTesting sem2: testing sem with higher amount ...\n");
        retval = sem2();
        if (retval == 0)                      { printf("===> sem2 successful!\n"); } 
        else                                  { printf("===> sem2 failed!\n");  return -1;}
    }

    if (SEM3)
    {
        printf("\nTesting sem3: test exiting while threads still sleeping...\n");
        retval = sem3();
        if (retval == 0)                      { printf("===> sem3 only successful when F12 shows no threads still remain!\n"); } 
        else                                  { printf("===> sem3 failed!\n");  return -1;}
    }

  if (SEM7)
  {
    printf("\nTesting sem7: multithread read, write to 1 same global variable...\n");
    retval = sem7();
    if (retval == 0)                      { printf("===> sem7 only successful when F12 shows no threads still remain!\n"); }
    else                                  { printf("===> sem7 failed!\n");  return -1;}
  }

  if (SEM9)
  {
    printf("\nTesting sem9: test sem init with value > 1, 2 threads w 2 sem_wait for each other...\n");
    retval = sem9();
    if (retval == 0)                      { printf("===> sem9 only successful when F12 shows no threads still remain!\n"); }
    else                                  { printf("===> sem9 failed!\n");  return -1;}
  }

    printf("\n\n---All tests completed! (press F12 to make sure no threads still alive)---\n");
    return 0;
}