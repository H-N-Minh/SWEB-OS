#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int cond1();

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success


// set to 1 to test, 0 to skip
#define COND1 1  
// #define cond2 0
// #define cond3 0
// #define cond4 1     
// #define cond5 0     // all 4 & 5 & 6 requires lot of physical memory so each should be tested alone
// #define cond6 0

int main()
{
    int retval = 0;

    if (COND1)
    {
        retval = cond1();
        if (retval == 0)                      { printf("cond1 successful!\n"); } 
        else                                  { printf("cond1 failed!\n");  return -1;}
    }

    // if (FORK2)
    // {
    //     retval = fork2();
    //     if (retval == PARENT_SUCCESS)         { printf("fork2 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("fork2 failed!\n"); return -1;}
    // }

    // if (FORK3)
    // {
    //     retval = fork3();
    //     if (retval == PARENT_SUCCESS)         { printf("fork3 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("fork3 failed!\n"); return -1;}
    // }

    // if (FORK4)
    // {
    //     retval = fork4();
    //     if (retval == PARENT_SUCCESS)         { printf("fork4 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("fork4 failed!\n"); return -1;}
    // }

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
    
    printf("testing completed\n");
    return 0;
}