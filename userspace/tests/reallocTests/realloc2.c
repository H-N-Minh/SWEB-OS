#include "assert.h"
#include "stdlib.h"
#include "string.h"

void check(int* ptr, int value, int n)
{
  for(int i = 0; i < n; i++)
  {
    if(ptr[i] != value)
    {
      printf("value: %d vs ptr[i] %d", value, ptr[i]);
      assert(0);
    }
  }
}

void set(int* ptr, int value, int n)
{
  for(int i = 0; i < n; i++)
  {
    ptr[i] = value;
  }
}
int realloc2()
{
  void* break_before = sbrk(0);

  //Test that realloc reuses space after free
  int* ptr1 = malloc(500 * sizeof(int));
  set(ptr1, 1, 500);
  int* ptr2 = malloc(400 * sizeof(int));
  set(ptr2, 2, 400);
  int* ptr3 = malloc(400 * sizeof(int));
  set(ptr3, 3, 400);
  int* ptr4 = malloc(300 * sizeof(int));
  set(ptr4, 4, 300);
  assert(ptr1 != NULL && ptr2 != NULL && ptr3 != NULL  && ptr4 != NULL );
  free(ptr3);

  ptr2 = realloc(ptr2, 700 * sizeof(int));
  check(ptr2, 2, 300);

  set(ptr2, 2, 600);
  assert(ptr2 != NULL);
  assert(ptr2 < ptr4);

  check(ptr1, 1, 500);
  // check(ptr2, 2, 700);
  check(ptr4, 4, 300);
  
  //test that realloc also reuses split up space
  
  
  int* ptr5 = malloc(300 * sizeof(int));
  printf("hey");
  int* ptr6 = malloc(300 * sizeof(int));
  printf("hey");
  int* ptr7 = malloc(300 * sizeof(int));
  printf("hey");
  assert(ptr5 != NULL && ptr6 != NULL && ptr7 != NULL );
  free(ptr5);
  free(ptr6);
   printf("hey");
  ptr4 = realloc(ptr4, 700 * sizeof(int));
  assert(ptr4 < ptr7);
  assert(ptr4 != NULL);


  free(ptr4);
  free(ptr1);
  free(ptr2);

  free(ptr7);


  void* break_after = sbrk(0);
  assert(break_before - break_after == 0);
  return 0;
}



	// int* ptrx = realloc(ptr, n * sizeof(int));
  // int* ptrx = malloc(n * sizeof(int));
  // int* ptrx = calloc(n, sizeof(int));