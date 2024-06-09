#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"

#define MEGABYTE 1048576
#define PAGESIZE 4096

#define N 7
#define N_MEGABYTE N * MEGABYTE


#define ELEMENTS_IN_ARRAY N_MEGABYTE / 8
#define PAGES_IN_ARRAY N_MEGABYTE/PAGESIZE


size_t big_array5[ELEMENTS_IN_ARRAY];  //5 Megabyes



//Show that pages only get swapped out if the get changed - TODOs add disk writes
int ps1()
{
  size_t tmp;
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    tmp = big_array5[i * (PAGESIZE / 8)];  
    printf("i %d", i); 
  }

  tmp = 1;
  assert(tmp == 1);

  return 0;
}