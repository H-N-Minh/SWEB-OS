#include "assert.h"
#include "stdlib.h"

#define BYTE 8
#define GIGABYTE 1000 * 8

//check that pages get only allocated on demand for malloc and right now for calloc
int malloc4()
{
  //TODOs syscall get number free pages
  int* ptr = malloc(50 * GIGABYTE * sizeof(int));
  //Check that is about the same
  printf("check 1\n");
  sleep(1);
  //Pages only get allocated when I actually use them
  for(int i = 0; i < 50 * GIGABYTE; i++)
  {
    ptr[i] = 1;
  }
  //check free pages is now smaller
  free(ptr);
  printf("check free\n");
  sleep(1);

  //For calloc all pages get allocated on initialization
  //TODOs syscall get number free pages
  int* ptr2 = (int*)calloc(50 * GIGABYTE, sizeof(int));
  printf("check 2\n");
  sleep(1);
  for(int i = 0; i < 50* GIGABYTE; i++)
  {
    assert(ptr2[i] == 0);
  }
  //check that is big
  free(ptr2);

  return 0;
}