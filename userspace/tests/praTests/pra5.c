#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"


#define PAGESIZE 4096
#define PAGES_IN_ARRAY 2000 
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE)/8

size_t big_array5[ELEMENTS_IN_ARRAY];  //5 Megabyes



//Trigger out of memory
int pra5()
{
  printf("...Write once in the array\n");
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array5[i * (PAGESIZE / 8)] = (size_t)i;
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array5[i * (PAGESIZE / 8)] == i);
  }
  printf("...Write in the same array again\n");
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array5[i * (PAGESIZE / 8)] = (size_t)i * 39;
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array5[i * (PAGESIZE / 8)] == i * 39);
  }
  printf("...Write in the same array again\n");
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array5[i * (PAGESIZE / 8)] = (size_t)i * 3;
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array5[i * (PAGESIZE / 8)] == i * 3);
  }
  
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array5[i * (PAGESIZE / 8)] == i * 3);
  }
  
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array5[i * (PAGESIZE / 8)] == i * 3);
  }
  printf("...Write in the same array again\n");
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array5[i * (PAGESIZE / 8)] = (size_t)i * 11;
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array5[i * (PAGESIZE / 8)] == i * 11);
  }

  return 0;
}