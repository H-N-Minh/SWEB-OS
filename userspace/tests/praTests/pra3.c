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


size_t big_array3[ELEMENTS_IN_ARRAY];  //5 Megabyes



//change PRA in every 100 pages
int pra3()
{
  setPRA(0);  // 0 for random pra, 1 for NFU
  int counter = 0;
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array3[i * (PAGESIZE / 8)] = (size_t)i;
    if (i%100 == 0)
    {
      setPRA(counter % 2);
      counter++;
    }
    
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array3[i * (PAGESIZE / 8)] == i);
    if (i%100 == 0)
    {
      setPRA(counter % 2);
      counter++;
    }
  }

  return 0;
}