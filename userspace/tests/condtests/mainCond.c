#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int cond1();
extern int cond2();
extern int cond3();
extern int cond4();

// set to 1 to test, 0 to skip
#define COND1 1         // simple test where child has to wait for parent's signal
#define COND2 1         // similar to cond1, but with more conds and both has to wait for each other
#define COND3 1         // test large number of threads on same cond, not working, not sure because of bad test or bad cond
#define COND4 1         // testing broadcast
// #define cond5 0      // testing wrong para
// #define cond6 0      // testing lost wake call

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

    // if (FORK5)
    // {
    //     retval = fork5();
    //     if (retval == PARENT_SUCCESS)         { printf("fork5 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("fork5 failed!\n"); return -1;}
    // }

    // if (FORK6)
    // {
    //     retval = fork6();
    //     if (retval == PARENT_SUCCESS)         { printf("fork6 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("fork6 failed!\n"); return -1;}
    // }
    
    printf("All tests completed!\n");
    return 0;
}