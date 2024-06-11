#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"

extern int ps1();
extern int ps2();

//set the test you want to test to 1
int PS1 = 1;
int PS2 = 1;



int main()
{
    // setPRA(__RANDOM_PRA__); 
    setPRA(__NFU_PRA__); 
    // setPRA(__SECOND_CHANCE_PRA__); 

    int retval;

    if (PS1)
    {
        printf("\nTesting ps1: Unchanged pages get not swapped out...\n");
        retval = ps1();
        if (retval == 0)                      { printf("===> ps1 successful!\n"); }
        else                                  { printf("===> ps1 failed!\n");  assert(0);}
    }

    if (PS2)
    {
        printf("\nTesting ps2: Test that pages that havent been changed since the last\n             swap out dont get written to the disk again but reuse the same ...\n");
        retval = ps2();
        if (retval == 0)                      { printf("===> ps2 successful!\n"); }
        else                                  { printf("===> ps2 failed!\n");  assert(0);}
    }


    printf("\n\n PageSelection testcases successful\n");
}
