#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


extern int random1();

int RANDOM1 = 1;

int main()
{
    int retval;
    if (RANDOM1)
    {
        printf("\nTesting random1: testing Random PRA is actually random...\n");
        retval = random1();
        if (retval == 0)                      { printf("===> random1 successful!\n"); }
        else                                  { printf("===> random1 failed!\n");  return -1;}
    }
                    
    return 0;

}
