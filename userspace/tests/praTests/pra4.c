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

  // printf("last in page %zu\n", (size_t) &big_array4[PAGESIZE/4 - 1]);
  // size_t* last_int_in_page = (size_t*) &big_array4[PAGESIZE/4 - 1];
  // *last_int_in_page = 11111111111111112; // cast to size_t to overflow into next page
  // printf("last in page2 %zu\n", *last_int_in_page);
  // printf("last in page3 %d\n", big_array4[PAGESIZE/4 - 1]);
  // printf("last in page4 %d\n", big_array4[PAGESIZE/4]);
  // printf("last in page5 %d\n", big_array4[PAGESIZE/4 + 1]);
  // printf("last in page6 %d\n", big_array4[PAGESIZE/4 - 2]);
  // return 0;

  for(int i = 0; i < PAGES_IN_ARRAY4; i++)
  {
    size_t* temp = (size_t*) (top_page + i * PAGESIZE4);
    *temp = i * 11111111111 + (i + 1);      // cast to size_t to overflow into next page
    i++;
  }

  printf("done writing phew!\n ");

  for(int i = 0; i < PAGES_IN_ARRAY4; i++)
  {
    size_t* temp = (size_t*) (top_page + i * PAGESIZE4);
    assert(*temp == i * 11111111111 + (i + 1));
    i++;
    printf("i: %d\n", i);
    
  }

  getPRAstats(&hit, &miss);
  printf("NFU PRA: Hit: %d, Miss: %d (after test)\n", hit, miss);

  return 0;
}