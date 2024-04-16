#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int sbrk1();
// extern int malloc1();
// extern int malloc2();
// extern int malloc3();
// extern int malloc4();
// extern int malloc5();
// extern int malloc6();


// set to 1 to test, 0 to skip
// TODO: ask if we would get point for implementing brk if we already have sbrk
#define SBRK1 1     //

#define MALLOC1 0     //
#define MALLOC2 0     //
#define MALLOC3 0     // 
#define MALLOC4 0     // 
#define MALLOC5 0     // 
#define MALLOC6 0     // 

int main()
{
    int retval = 0;

    if (SBRK1)
    {
        retval = sbrk1();
        if (retval == 0)                         { printf("sbrk1 successful!\n"); } 
        else if (retval == -1)                   { return 0; }                      // this kills all child processes
        else                                     { printf("sbrk1 failed!\n");  return -1;}
    }

    // if (MALLOC1)
    // {
    //     retval = malloc1();
    //     if (retval == 0)         { printf("malloc1 successful!\n"); } 
    //     else if (retval == -1)     { return 0; }                      // this kills all child processes
    //     else                                  { printf("malloc1 failed!\n");  return -1;}
    // }

    // if (MALLOC2)
    // {
    //     retval = malloc2();
    //     if (retval == PARENT_SUCCESS)         { printf("malloc2 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("malloc2 failed!\n"); return -1;}
    // }

    // if (MALLOC3)
    // {
    //     retval = malloc3();
    //     if (retval == PARENT_SUCCESS)         { printf("malloc3 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("malloc3 failed!\n"); return -1;}
    // }

    // if (MALLOC4)
    // {
    //     retval = malloc4();
    //     if (retval == PARENT_SUCCESS)         { printf("malloc4 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("malloc4 failed!\n"); return -1;}
    // }

    // if (MALLOC5)
    // {
    //     retval = malloc5();
    //     if (retval == PARENT_SUCCESS)         { printf("malloc5 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("malloc5 failed!\n"); return -1;}
    // }

    // if (MALLOC6)
    // {
    //     retval = malloc6();
    //     if (retval == PARENT_SUCCESS)         { printf("malloc6 successful!\n"); } 
    //     else if (retval == CHILD_SUCCESS)     { return 0; }                      
    //     else                                  { printf("malloc6 failed!\n"); return -1;}
    // }

    return 0;
}