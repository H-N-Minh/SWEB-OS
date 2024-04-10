#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int sem1();
extern int sem2();
// extern int cond3();
// extern int cond4();
// extern int cond5();
// extern int cond6();

// set to 1 to test, 0 to skip
#define SEM1 0         // simple test
#define SEM2 1         // cond and sem together
// #define COND3 1         // test large number of threads on same cond
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
        printf("\nTesting sem2: simple test...\n");
        retval = sem2();
        if (retval == 0)                      { printf("===> sem2 successful!\n"); } 
        else                                  { printf("===> sem2 failed!\n");  return -1;}
    }

    // if (COND3)
    // {
    //     printf("\nTesting cond3: large amount of threads on same Cond...\n");
    //     retval = cond3();
    //     if (retval == 0)                      { printf("===> cond3 successful!\n"); } 
    //     else                                  { printf("===> cond3 failed!\n");  return -1;}
    // }

    // if (COND4)
    // {
    //     printf("\nTesting cond4: broadcasting large amount of threads...\n");
    //     retval = cond4();
    //     if (retval == 0)                      { printf("===> cond4 successful!\n"); } 
    //     else                                  { printf("===> cond4 failed!\n");  return -1;}
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
    
    printf("\n\n---All tests completed!---\n");
    return 0;
}