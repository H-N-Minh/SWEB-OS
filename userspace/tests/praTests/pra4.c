#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"

#define MEGABYTE4 1048576
#define PAGESIZE4 4096

#define N4 5
#define N_MEGABYTE4 N4 * MEGABYTE4


#define ELEMENTS_IN_ARRAY4 N_MEGABYTE4 / 4
#define PAGES_IN_ARRAY4 N_MEGABYTE4/PAGESIZE4


int big_array4[ELEMENTS_IN_ARRAY4];  //5 Megabyes

size_t getTopOfThisPage(size_t variable_adr)
{
  size_t top_stack = variable_adr - variable_adr%PAGESIZE4 + PAGESIZE4 - sizeof(int);
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

// writting a big number at the end of a page that it overflows to the next page, causes double pagefault at a time
int pra4()
{
  setPRA(__NFU_PRA__); 
  int hit;
  int miss;
  getPRAstats(&hit, &miss);
  
  printf("NFU PRA: Hit: %d, Miss: %d (before test)\n", hit, miss);

  size_t top_page = getTopOfThisPage((size_t) big_array4);
  
  size_t* temp = (size_t*) top_page;
  for(size_t i = 0; i < (PAGES_IN_ARRAY4 / 2); i++)
  {
    *temp = (size_t) (i * 10000000000 + (i + 1));      // cast to size_t to overflow into next page
    temp = (size_t*) ((size_t) temp + PAGESIZE4 * 2);
  }

  printf("done writing phew!\n ");

  temp = (size_t*) top_page;
  for(size_t i = 0; i < (PAGES_IN_ARRAY4 / 2); i++)
  {
    printf("%ld\n",i);
    if (*temp != (size_t) (i * 10000000000 + (i + 1)))
    {
      printf("Error: Expected %zu, got %zu. At index %zu\n", (size_t) (i * 10000000000 + (i + 1)), *temp, i);
    }
    temp = (size_t*) ((size_t) temp + PAGESIZE4 * 2);
  }

  getPRAstats(&hit, &miss);
  printf("NFU PRA: Hit: %d, Miss: %d (after test)\n", hit, miss);

  return 0;
}