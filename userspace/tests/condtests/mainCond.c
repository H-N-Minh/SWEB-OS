#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int cond1();
extern int cond2();
extern int cond3();
extern int cond4();
extern int cond5();
extern int cond6();

//NOTE COND3 and COND4 only works with very small number of threads, not sure if the problem with our fake mutex or bad cond
// TODO: test these 2 again with bigger amount when mutex is working

// set to 1 to test, 0 to skip
#define COND1 1         // simple test where child has to wait for parent's signal
#define COND2 1         // similar to cond1, but with more conds and both has to wait for each other
#define COND3 1         // test large number of threads on same cond
#define COND4 1         // testing broadcast
#define COND5 1         // testing wrong para
#define COND6 0         // testing lost wake call

int main()
{
    int retval = 0;

    if (COND1)
    {
        printf("Testing cond1...");
        retval = cond1();
        if (retval == 0)                      { printf("=> cond1 successful!\n"); } 
        else                                  { printf("=> cond1 failed!\n");  return -1;}
    }

    if (COND2)
    {
        printf("Testing cond2...");
        retval = cond2();
        if (retval == 0)                      { printf("=> cond2 successful!\n"); } 
        else                                  { printf("=> cond2 failed!\n");  return -1;}
    }

    if (COND3)
    {
        printf("Testing cond3...");
        retval = cond3();
        if (retval == 0)                      { printf("=> cond3 successful!\n"); } 
        else                                  { printf("=> cond3 failed!\n");  return -1;}
    }

    if (COND4)
    {
        printf("Testing cond4...");
        retval = cond4();
        if (retval == 0)                      { printf("=> cond4 successful!\n"); } 
        else                                  { printf("=> cond4 failed!\n");  return -1;}
    }

    if (COND5)
    {
        printf("Testing cond5...");
        retval = cond5();
        if (retval == 0)                      { printf("=> cond5 successful!\n"); } 
        else                                  { printf("=> cond5 failed!\n");  return -1;}
    }

    if (COND6)
    {
        printf("Testing cond6...");
        retval = cond6();
        if (retval == 0)                      { printf("=> cond6 successful!\n"); } 
        else                                  { printf("=> cond6 failed!\n");  return -1;}
    }
    
    printf("All tests completed!\n");
    return 0;
}