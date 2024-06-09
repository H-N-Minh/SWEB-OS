#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"


#define PAGESIZE 4096


#define PAGES_IN_ARRAY 1500
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE) / 8

size_t big_array5[ELEMENTS_IN_ARRAY];

void printStatistic1()
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


//Test: Unchanged pages get not swapped out.
int ps1()
{
  size_t tmp;
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    tmp = big_array5[i * (PAGESIZE / 8)];  
  }

  tmp = 1;
  assert(tmp == 1);

  printStatistic1();

  int disk_writes;
  int disk_reads;
  int discard_unchanged_page;
  int reuse_same_disk_location;
  getSwappingStats(&disk_writes, &disk_reads, &discard_unchanged_page, &reuse_same_disk_location);
  assert(disk_writes < 10 && disk_reads < 10);
  assert(discard_unchanged_page >= 500 && "Basically all swap out should be discarded since the page is unchanged and can be loaded back in again from the disk");
  return 0;
}


