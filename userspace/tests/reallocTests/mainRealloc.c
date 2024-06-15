#include "stdio.h"
#include "unistd.h"
#include "assert.h"


extern int realloc1();
extern int realloc2();

#define REALLOC1 1
#define REALLOC2 1

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

   
    if(failcounter == 0)
    {
        printf("\n\n---All tests completed sucessfully!\n");
    }
    return 0;
}