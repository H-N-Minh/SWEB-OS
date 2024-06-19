#include "stdio.h"
#include "unistd.h"
#include "assert.h"


extern int realloc1();
extern int realloc2();
extern int realloc3();

#define REALLOC1 1
#define REALLOC2 1
#define REALLOC3 1

int main()
{
    int failcounter = 0;

    int retval = 0;
    if (REALLOC1)
    {
        printf("\nTesting realloc1: ...\n");
        retval = realloc1();
        if (retval == 0)                      { printf("===> realloc1 successful!\n"); } 
        else                                  { printf("===> realloc1 failed!\n");  return -1;}
    }

    if (REALLOC2)
    {
        printf("\nTesting realloc2: ...\n");
        retval = realloc2();
        if (retval == 0)                      { printf("===> realloc2 successful!\n"); } 
        else                                  { printf("===> realloc2 failed!\n");  return -1;}
    }
    if (REALLOC3)
    {
        printf("\nTesting realloc2: Calloc with swapping\n");
        retval = realloc3();
        if (retval == 0)                      { printf("===> realloc3 successful!\n"); } 
        else                                  { printf("===> realloc3 failed!\n");  return -1;}
    }

   
    if(failcounter == 0)
    {
        printf("\n\n---All tests completed sucessfully!\n");
    }
    return 0;
}