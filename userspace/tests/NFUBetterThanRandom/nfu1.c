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


//Test for which nfu works better than random - aka working locally
int main()
{
  //Set the pra to the one you want to test
  //  setPRA(__RANDOM_PRA__); 
    setPRA(__NFU_PRA__); 

  //For random pra I get on my device:
  //----------------------------
  // Total disk writes: 681
  // Total disk reads: 20
  // Discard unchanged page 3
  // Reuse same disk location 0

  //For nfu pra I get on my device:
  //----------------------------
  // Total disk writes: 665
  // Total disk reads: 3
  // Discard unchanged page 0
  // Reuse same disk location 0

  printf("Testing...\n");
  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;  
    }
  }

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }
  for(int i = 0; i < 20; i++)
  {
    for(int j = 500; j < 520; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = i;
    }
  }
  
  calloc(800, 4096);

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;  
    }
  }

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = i;
    }
  }

 calloc(800, 4096);

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = (size_t)j * 14;  
    }
  }

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }

  for(int i = 0; i < 20; i++)
  {
    for(int j = 20; j < 25; j++)
    {
      big_array[j * (PAGESIZE / 8)] = i;
    }
  }
  for(int i = 0; i < 20; i++)
  {
    for(int j = 500; j < 520; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }

   calloc(800, 4096);

    for(int i = 0; i < 20; i++)
  {
    for(int j = 500; j < 520; j++)
    {
       big_array[j * (PAGESIZE / 8)] = (size_t)j * 13;    
    }
  }

  


  printStatistic();

  printf("\nTestcase finished! (change setPra to test the other pra)\n");

  return 0;
}





