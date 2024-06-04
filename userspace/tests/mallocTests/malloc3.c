#include "assert.h"
#include "stdlib.h"

//Test reusing after free (and splitting and concatenating freed space for reuse)
int malloc3()
{
  void* break_before = sbrk(0);
  void* ptr1 = malloc(30 * sizeof(int));
  void* ptr2 = malloc(20 * sizeof(int));
  assert(ptr1 < ptr2);

  //Test if freed space after free gets reused
  free(ptr1);
  void* ptr3 = malloc(20 * sizeof(int));
  assert(ptr3 < ptr2 && "If the address of ptr3 is not smaller than ptr2, then the space was not reused");

  //Test if freed space get split into smaller segments
  free(ptr3);
  void* ptr4 = malloc(2 * sizeof(int));
  void* ptr5 = malloc(2 * sizeof(int));
  assert(ptr4 < ptr2 && "If the address of ptr4 is not smaller than ptr2, then the space was not reused");
  assert(ptr5 < ptr2 && "If the address of ptr5 is not smaller than ptr2, then the space was not reused");


  free(ptr2);
  free(ptr4);
  free(ptr5);

  void* break_after = sbrk(0);
  assert(break_before - break_after == 0);


  //Test if freed space after many small malloc calls can be reused for bigger malloc calls
  void* ptr6 = malloc(2 * sizeof(int));
  void* ptr7 = malloc(2 * sizeof(int));
  void* ptr8 = malloc(2 * sizeof(int));
  void* ptr9 = malloc(2 * sizeof(int));
  void* ptr10 = malloc(2 * sizeof(int));

  assert(ptr6 < ptr7 && ptr7 < ptr8 && ptr8 < ptr9 && ptr9 < ptr10);

  free(ptr8);
  free(ptr7);
  free(ptr9);

  void* ptr11 = malloc(5 * sizeof(int));

  assert(ptr11 < ptr10 && "If new ptr is bigger it has not reused the space");


  free(ptr11);
  free(ptr6);
  free(ptr10);

  void* break_end = sbrk(0);
  assert(break_end - break_after == 0);











  return 0;
}