#include "assert.h"
#include "stdlib.h"
#include "sched.h"

#define NUM_THREADS 60
#define NUM_MALLOC_CALLS 60

int all_threads_created = 0;

int function_5(int unique_number)
{
  while(!all_threads_created){sched_yield();}

  void* ptrs[NUM_MALLOC_CALLS];
  for(int i = 0; i < NUM_MALLOC_CALLS; i++)
  {
    ptrs[i] = malloc((i+1) * sizeof(int));
    assert(ptrs[i] != NULL);
  }

  for(int i = 0; i < NUM_MALLOC_CALLS; i++)
  {
    int* ptr = ptrs[i];
    for(int j = 0; j < (i+1); j++)
    {
      ptr[j] = j + unique_number;
    }
  }

  for(int i = 0; i < NUM_MALLOC_CALLS; i++)
  {
    int* ptr = ptrs[i];
    for(int j = 0; j < (i+1); j++)
    {
      assert(ptr[j] == j + unique_number);
    }
  }

  for(int i = 0; i < NUM_MALLOC_CALLS; i++)
  {
    free(ptrs[i]);
  }
  return 0;
}



//Simple Test for malloc with multithreading
int malloc5()
{
  void* break_before = sbrk(0);
  pthread_t thread_id[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++)
  {
    int rv = pthread_create(&thread_id[i], NULL, (void*)function_5, (void*)((long)i));
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