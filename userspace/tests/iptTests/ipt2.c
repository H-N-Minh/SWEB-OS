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


size_t big_array[ELEMENTS_IN_ARRAY];  //5 Megabyes



//Trigger out of memory
int ipt2()
{
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array[i * (PAGESIZE / 8)] = (size_t)i;
    if (i%100 == 0)
    {
      assertIPT();
    }
    
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array[i * (PAGESIZE / 8)] == i);
    if (i%100 == 0)
    {
      assertIPT();
    }
    
  }

  return 0;
}