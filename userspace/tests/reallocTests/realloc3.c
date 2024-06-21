#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"


#define PAGESIZE 4096


#define PAGES_IN_ARRAY 2000
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE) / 8


size_t big_array[ELEMENTS_IN_ARRAY];

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


//Heap and swapping
int realloc3()
{
  //Set the pra to the one you want to test
  //  setPRA(__RANDOM_PRA__); 
  // setPRA(__NFU_PRA__); 

  int* ptr1 = calloc(800, 4096);
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
      big_array[i * (PAGESIZE / 8)] = (size_t)i * 14;  
    
  }
  int* ptr2 = calloc(200, 4096);
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array[i * (PAGESIZE / 8)] == (size_t)i* 14);
    
  }

  free(ptr2);
  free(ptr1);

  printStatistic();

  return 0;
}
