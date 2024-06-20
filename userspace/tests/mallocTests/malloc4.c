#include "assert.h"
#include "stdlib.h"
#include "nonstd.h"

#define BYTE 8
#define GIGABYTE 1000 * 8

//check that pages get only allocated on demand for malloc and right now for calloc
int malloc4()
{
  int* ptr = malloc(100 * GIGABYTE * sizeof(int));
  //Check that is about the same
  int free_pages = getNumFreePages();
  //printf("Free pages %d\n", getNumFreePages());
  assert(free_pages > 950);
  
  //Pages only get allocated when I actually use them
  for(int i = 0; i < 100 * GIGABYTE; i++)
  {
    ptr[i] = 1;
  }
  //check free pages is now smaller
  int free_pages_2 = getNumFreePages();
  // printf("Free pages %d\n", getNumFreePages());
  assert(free_pages_2 < free_pages && free_pages_2 < 500);
  free(ptr);
  int free_pages_3 = getNumFreePages();
  assert(free_pages_3 > 950);
  
  

  //For calloc all pages get allocated on initialization
  int* ptr2 = (int*)calloc(100 * GIGABYTE, sizeof(int));
  int free_pages_4 = getNumFreePages();
  // printf("Free pages %d\n", getNumFreePages());
  assert(free_pages_4 < 500);
  for(int i = 0; i < 100 * GIGABYTE; i++)
  {
    assert(ptr2[i] == 0);
  }
  //check that is big
  free(ptr2);

  return 0;
}