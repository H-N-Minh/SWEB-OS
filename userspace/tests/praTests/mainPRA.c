#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"

void printStatistic()
{
  int disk_writes;
  int disk_reads;
  int discard_unchanged_page;
  int reuse_same_disk_location;
  getSwappingStats(&disk_writes, &disk_reads, &discard_unchanged_page, &reuse_same_disk_location);
  printf("------------------------------------------------\n");
  printf("Total disk writes: %d\nTotal disk reads: %d\nDiscard unchange pages %d\nReuse same disk location %d\n", disk_writes, disk_reads, discard_unchanged_page, reuse_same_disk_location);
  printf("------------------------------------------------\n");
}

extern int pra1();
extern int pra2();
extern int pra3();
extern int pra4();
extern int pra5();



int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run
    int PRA1 = 1;   // Test PRA
    int PRA2 = 1;       // double pagefault => double swap (still single thread)
    int PRA3 = 1;       // 64 threads  writting to array at same time                //seems to fail - mayge caused by new fix ???
    int PRA4 = 0;       // similar to pra3, but use fork instead of pthread_create
    int PRA5 = 0;       //More complex out of memory where i write in every location multiple times
    // Select pra-type:
    // setPRA(__RANDOM_PRA__); 
    // setPRA(__NFU_PRA__); 
    // setPRA(__SECOND_CHANCE_PRA__); 

    if (PRA1)
    {
        printf("\nTesting pra1: Trigger out of memory...\n");
        retval = pra1();
        if (retval == 0)                      { printf("===> pra1 successful!\n"); }
        else                                  { printf("===> pra1 failed!\n");  return -1;}
        printStatistic();
    }

    if (PRA2)
    {
        printf("\nTesting pra2: testing double pagefault => double swap...\n");
        retval = pra2();
        if (retval == 0)                      { printf("===> pra2 successful!\n"); }
        else                                  { printf("===> pra2 failed!\n");  return -1;}
        printStatistic();
    }

    for(int i = 0; i < 3; i++)
    {
    if(PRA3)
    {
        printf("\nTesting pra3: testing 64 threads writing to array in parallel...\n");
        retval = pra3();
        if (retval == 0)                      { printf("===> pra3 successful!\n"); }
        else                                  { printf("===> pra3 failed!\n");  return -1;}
        printStatistic();
    }
    }


    if (PRA4)
    {
        printf("\nTesting pra4: Many forks and children trigger out of memory...\n");
        retval = pra4();
        if (retval == 0)                      { printf("===> pra4 successful!\n"); }
        else                                  { printf("===> pra4 failed!\n");  return -1;}
        printStatistic();
    }

    if(PRA5)
    {
        printf("\nTesting pra5: Out of memory but more complex...\n");
        retval = pra5();
        if (retval == 0)                      { printf("===> pra5 successful!\n"); }
        else                                  { printf("===> pra5 failed!\n");  return -1;}
        printStatistic();
    }


    printf("\n\n===  All pra testcases successful  ===\n");
    return 0;
}

int main()
{
    int child_exit_code = childMain();
    printf("returnvalue %d", child_exit_code);
   
    return 0;
}


 