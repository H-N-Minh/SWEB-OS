#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"


#define PAGESIZE 4096
#define PAGES_IN_ARRAY 1280 
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE)/8

size_t big_array1[ELEMENTS_IN_ARRAY];  //5 Megabyes



//Trigger out of memory
int pra1()
{
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array1[i * (PAGESIZE / 8)] = (size_t)i;
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array1[i * (PAGESIZE / 8)] == i);
  }

  return 0;
}