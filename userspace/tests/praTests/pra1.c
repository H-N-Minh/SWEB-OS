#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"

#define MEGABYTE 1048576
#define PAGESIZE 4096

#define N 5
#define N_MEGABYTE N * MEGABYTE


#define ELEMENTS_IN_ARRAY N_MEGABYTE / 8
#define PAGES_IN_ARRAY N_MEGABYTE/PAGESIZE


size_t big_array1[ELEMENTS_IN_ARRAY];  //5 Megabyes



//Trigger out of memory
int pra1()
{
  setPRA(__RANDOM_PRA__); 
  int hit;
  int miss;
  getPRAstats(&hit, &miss);
  printf("Random PRA: Hit: %d, Miss: %d (before test)\n", hit, miss);

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array1[i * (PAGESIZE / 8)] = (size_t)i;
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array1[i * (PAGESIZE / 8)] == i);
  }
  getPRAstats(&hit, &miss);
  printf("Random PRA: Hit: %d, Miss: %d (after test)\n", hit, miss);

  return 0;
}