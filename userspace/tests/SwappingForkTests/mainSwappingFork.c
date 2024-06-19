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

extern int swappingFork1();
extern int swappingFork2();
extern int swappingFork3();



int main()
{
    int retval = 0;

    // comment out the tests you don't want to run
    int SWAPPINGFORK1 = 1;    //need to be tested alone otherwise to much kernelheap usage
    int SWAPPINGFORK2 = 0;     
    int SWAPPINGFORK3 = 0;    
  
    // Select pra-type:
    // setPRA(__RANDOM_PRA__); 
    // setPRA(__NFU_PRA__); 
    // setPRA(__SECOND_CHANCE_PRA__); 

    if (SWAPPINGFORK1)
    {
        printf("\nTesting swappingFork1: Parent starts swapping and than parent and child write in the same big array.\n");
        retval = swappingFork1();
        if (retval == 0)                      { printf("===> swappingFork1 successful!\n"); }
        else                                  { printf("===> swappingFork1 failed!\n");  return -1;}
        printStatistic();
    }

    if (SWAPPINGFORK2)
    {
        printf("\nTesting swappingFork2: Many children write so much that we start swapping.\n");
        retval = swappingFork2();
        if (retval == 0)                      { printf("===> swappingFork2 successful!\n"); }
        else                                  { printf("===> swappingFork2 failed!\n");  return -1;}
        printStatistic();
    }


    if(SWAPPINGFORK3)
    {
        printf("\nTesting swappingFork3: Fork after swapping - only makes sense if we also run the previous test.\n");
        retval = swappingFork3();
        if (retval == 0)                      { printf("===> swappingFork3 successful!\n"); }
        else                                  { printf("===> swappingFork3 failed!\n");  return -1;}
        printStatistic();
    }





    printf("\n\n===  All swapping fork testcases successful  ===\n");
    return 0;
}



 