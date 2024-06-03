#include "assert.h"
#include "stdlib.h"


//Test; Simple sanity check
int malloc1()
{
  void* ptr;
  void* break_before;
  void* break_after;

  //Malloc should return NULL and dont change the break if size of elements is null
  
  break_before = sbrk(0);
  ptr = malloc(NULL);
  break_after = sbrk(0);
  assert(ptr == NULL);
  assert(break_after - break_before == 0);

  //Calloc should return NULL and dont change the break if number of elements or size of elements is null
  break_before = sbrk(0);
  ptr = calloc(0, 0);
  break_after = sbrk(0);
  assert(ptr == NULL);
  assert(break_after - break_before == 0);

  //Calloc should return NULL and dont change the break if size of elements is null
  break_before = sbrk(0);
  ptr = calloc(3, 0);
  break_after = sbrk(0);
  assert(ptr == NULL);
  assert(break_after - break_before == 0);

  //Calloc should return NULL and dont change the break if number of elements is null
  break_before = sbrk(0);
  ptr = calloc(0, 4);
  break_after = sbrk(0);
  assert(ptr == NULL);
  assert(break_after - break_before == 0);

  //Free Null shouldnt change the break and not crash the problem
  break_before = sbrk(0);
  free(NULL);
  break_after = sbrk(0); 
  assert(break_after - break_before == 0);




  return 0;
}