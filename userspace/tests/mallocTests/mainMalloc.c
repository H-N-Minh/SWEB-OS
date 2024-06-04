#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int sbrk1();
extern int brk1();
extern int malloc1();
extern int malloc2();
extern int malloc3();
extern int malloc4();
extern int malloc5();
extern int malloc6();



// set to 1 to test, 0 to skip
#define SBRK1 1     // basic test for sbrk
#define BRK1 1     // basic test for brk
#define MALLOC1 1   // malloc simple sanity checks
#define MALLOC2 1 
#define MALLOC3 1
#define MALLOC4 1 
#define MALLOC5 1
#define MALLOC6 1

int main()
{
    int failcounter = 0;

    int retval = 0;
    if (SBRK1)
    {
        printf("\nTesting sbrk1: Basic sbrk test.\n");
        retval = sbrk1();
        if (retval == 0)                      { printf("===> sbrk1 successful!\n"); } 
        else                                  { printf("===> sbrk1 failed!\n");  return -1;}
    }

    if (BRK1)
    {
        printf("\nTesting brk1: Basic brk test.\n");
        retval = brk1();
        if (retval == 0)                      { printf("===> brk1 successful!\n"); } 
        else                                  { printf("===> brk1 failed!\n");  return -1;}
    }

    if (MALLOC1)
    {
        printf("\nTesting malloc1: Simple sanity checks.\n");
        retval = malloc1();
        if (retval == 0)                      { printf("===> malloc1 successful!\n"); } 
        else                                  { printf("===> malloc1 failed!\n");   return -1;}
    }

    if (MALLOC2)
    {
        printf("\nTesting malloc2: TODOs...\n");
        retval = malloc2();
        if (retval == 0)                      { printf("===> malloc2 successful!\n"); } 
        else                                  { printf("===> malloc2 failed!\n");   return -1;}
    }

    if (MALLOC3)
    {
        printf("\nTesting malloc3: Test reusing after free (and splitting\n                 and concatenating freed space for reuse).\n");
        retval = malloc3();
        if (retval == 0)                      { printf("===> malloc3 successful!\n"); } 
        else                                  { printf("===> malloc3 failed!\n");   return -1;}
    }

    if (MALLOC4)
    {
        printf("\nTesting malloc4: Check demand allocation.\n");           //Todos: syscall missing
        retval = malloc4();
        if (retval == 0)                      { printf("===> malloc4 successful!\n"); } 
        else                                  { printf("===> malloc4 failed!\n");   return -1;}
    }

    if (MALLOC5)
    {
        printf("\nTesting malloc5: Simple Test for malloc with multithreading.\n");
        retval = malloc5();
        if (retval == 0)                      { printf("===> malloc5 successful!\n"); } 
        else                                  { printf("===> malloc5 failed!\n");   return -1;}
    }

    if (MALLOC6)
    {
        printf("\nTesting malloc6: Malloc exits savely if its detect overflow\n                 when trying to allocate a new element.\n");
        retval = malloc6();
        if (retval == 0)                      { printf("===> malloc6 successful!\n"); } 
        else                                  { printf("===> malloc6 failed!\n");   return -1;}
    }

    if(failcounter == 0)
    {
        printf("\n\n---All tests completed sucessfully!\n");
    }
    return 0;
}