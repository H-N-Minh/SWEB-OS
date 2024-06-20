#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define NUM_THREADS 10
int all_threads_created = 0;

void check4(int* ptr, int value, int n)
{
  for(int i = 0; i < n; i++)
  {
    if(ptr[i] != value)
    {
      assert(0);
    }
  }
}

void set4(int* ptr, int value, int n)
{
  for(int i = 0; i < n; i++)
  {
    ptr[i] = value;
  }
}
int function4()
{

  //Test that realloc reuses space after free
  int* ptr1 = malloc(500 * sizeof(int));
  set4(ptr1, 1, 500);
  int* ptr2 = malloc(400 * sizeof(int));
  set4(ptr2, 2, 400);
  int* ptr3 = malloc(400 * sizeof(int));
  set4(ptr3, 3, 400);
  int* ptr4 = malloc(300 * sizeof(int));
  set4(ptr4, 4, 300);
  free(ptr3);

  ptr2 = realloc(ptr2, 700 * sizeof(int));
  check4(ptr2, 2, 300);

  set4(ptr2, 2, 700);
  assert(ptr2 != NULL);

  check4(ptr1, 1, 500);
  check4(ptr2, 2, 700);
  check4(ptr4, 4, 300);
  
  //test that realloc also reuses split up space
  
  
  int* ptr5 = malloc(300 * sizeof(int));
  int* ptr6 = malloc(300 * sizeof(int));
  int* ptr7 = malloc(300 * sizeof(int));
  free(ptr5);
  free(ptr6);
  ptr4 = realloc(ptr4, 700 * sizeof(int));
  assert(ptr4 != NULL);


  free(ptr4);
  free(ptr1);
  free(ptr2);

  free(ptr7);

  return 0;
}


int realloc4()
{
  void* break_before = sbrk(0);
  pthread_t thread_id[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++)
  {
    int rv = pthread_create(&thread_id[i], NULL, (void*)function4, (void*)((long)i));
    assert(rv == 0);
  }
  all_threads_created = 1;

  for(int i = 0; i < NUM_THREADS; i++)
  {
    void* retval;
    int rv = pthread_join(thread_id[i], &retval);
    assert(rv == 0);
    assert(retval == (void*)0);
  } 

  void* break_after = sbrk(0);   

  assert(break_after - break_before == 0);
  return 0;
}