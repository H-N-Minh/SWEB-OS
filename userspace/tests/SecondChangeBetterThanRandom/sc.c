#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"


#define PAGESIZE 4096

#define PAGES_IN_ARRAY2 1000
#define ELEMENTS_IN_ARRAY2 (PAGES_IN_ARRAY2 * PAGESIZE) / 8

#define PAGES_IN_ARRAY 50
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE) / 8
size_t array[ELEMENTS_IN_ARRAY];


size_t array2[ELEMENTS_IN_ARRAY2];

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


//Test for which nfu works better than random - aka working locally
int main()
{
  //Set the pra to the one you want to test
  //  setPRA(__RANDOM_PRA__); 
    setPRA(__SECOND_CHANCE_PRA__); 

  //For random pra I get on my device:
  //----------------------------
  //Total disk writes: 1146
  //Total disk reads: 62
  //Discard unchanged page: 3
  //Reuse same disk location: 1

  //For second change pra I get on my device:
  //----------------------------
  //Total disk writes: 1083
  //Total disk reads: 2
  //Discard unchanged page: 7
  //Reuse same disk location: 0

 
  for(int i = 0; i < PAGES_IN_ARRAY2; i++)
  {
    array2[i * (PAGESIZE / 8)] = (size_t)2;
  }

  for(int j = 0; j < 10; j++)
  {
    printf("%d", j);
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      array[i * (PAGESIZE / 8)] = (size_t)j;
    }
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      assert(array[i * (PAGESIZE / 8)] == j);
    }
    calloc(100, PAGESIZE);
  }



  printStatistic();

  printf("\nTestcase finished! (change setPra to test the other pra)\n");

  return 0;
}





