#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int sem1();
extern int sem2();
extern int sem3();
// extern int cond4();
// extern int cond5();
// extern int cond6();

// set to 1 to test, 0 to skip
#define SEM1 1                // simple test
#define SEM2 1                // cond and sem together
#define SEM3 1                // multiple threads waiting on same sem, also test exiting while threads still sleeping
// #define COND4 1         // testing broadcast
// #define COND5 1         // testing wrong para
// #define COND6 0         // testing lost wake call. This should be tested alone, check the file for details

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

    // if (SEM4)
    // {
    //     printf("\nTesting sem4: test multiple threads waiting on same sem...\n");
    //     retval = sem4();
    //     if (retval == 0)                      { printf("===> sem4 only successful when F12 shows no threads still remain!\n"); } 
    //     else                                  { printf("===> sem4 failed!\n");  return -1;}
    // }

    // if (COND5)
    // {
    //     printf("\nTesting cond5: testing wrong parameters...\n");
    //     retval = cond5();
    //     if (retval == 0)                      { printf("===> cond5 successful!\n"); } 
    //     else                                  { printf("===> cond5 failed!\n");  return -1;}
    // }

    // if (COND6)
    // {
    //     printf("\nTesting cond6: testing 'lost wake call'...\n");
    //     retval = cond6();
    //     if (retval == 0)                      { printf("===> cond6 is only successful when the order of step is correct!\n"); } 
    //     else                                  { printf("===> cond6 failed!\n");  return -1;}
    // }
    
    printf("\n\n---All tests completed! (press F12 to make sure no threads still alive)---\n");
    return 0;
}