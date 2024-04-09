#include "stdio.h"
#include "unistd.h"
#include "assert.h"

// extern int cond1();
// extern int cond2();
// extern int cond3();
// extern int cond4();
// extern int cond5();
// extern int cond6();

//NOTE COND3 and COND4 only works with very small number of threads, not sure if the problem with our fake mutex or bad cond
// TODO: test these 2 again with bigger amount when mutex is working

// set to 1 to test, 0 to skip
#define COND1 1         // simple test where child has to wait for parent's signal
#define COND2 1         // similar to cond1, but with more conds and both has to wait for each other
#define COND3 1         // test large number of threads on same cond
#define COND4 1         // testing broadcast
#define COND5 1         // testing wrong para
#define COND6 0         // testing lost wake call. This should be tested alone, check the file for details

int main()
{
    // int retval = 0;

    // if (COND1)
    // {
    //     printf("\nTesting cond1: simple test...\n");
    //     retval = cond1();
    //     if (retval == 0)                      { printf("===> cond1 successful!\n"); } 
    //     else                                  { printf("===> cond1 failed!\n");  return -1;}
    // }

    // if (COND2)
    // {
    //     printf("\nTesting cond2: more complicated test...\n");
    //     retval = cond2();
    //     if (retval == 0)                      { printf("===> cond2 successful!\n"); } 
    //     else                                  { printf("===> cond2 failed!\n");  return -1;}
    // }

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
    
    // printf("\n\n---All tests completed!---\n");
    return 0;
}