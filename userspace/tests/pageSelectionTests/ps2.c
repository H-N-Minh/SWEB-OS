#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"


#define PAGESIZE 4096


#define PAGES_IN_ARRAY 1500
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE) / 8


size_t big_array[ELEMENTS_IN_ARRAY];

void printStatistic2()
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


//Test that pages that havent been changed since the last swap dont get written to the disk again but reuse the same location
int ps2()
{
  int disk_writes_before;
  int disk_reads_before;
  int discard_unchanged_page_before;
  int reuse_same_disk_location_before;
  getSwappingStats(&disk_writes_before, &disk_reads_before, &discard_unchanged_page_before, &reuse_same_disk_location_before);



  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array[i * (PAGESIZE / 8)] = (size_t)i * 13;    
  }

  for(int i = 0; i < 3; i++)
  {
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      assert(big_array[i * (PAGESIZE / 8)] == (size_t)i * 13);    
    }
  }

  printStatistic2();

  int disk_writes;
  int disk_reads;
  int discard_unchanged_page;
  int reuse_same_disk_location;
  getSwappingStats(&disk_writes, &disk_reads, &discard_unchanged_page, &reuse_same_disk_location);

  
  assert((disk_reads - disk_reads_before) > (disk_writes - disk_writes_before) && "Disk reads need to be higher than disk writes, because we will read from the same location multiple times.");

  assert((reuse_same_disk_location - reuse_same_disk_location_before) > 1000 && "Number of reused disk pages should be big");


  return 0;
}