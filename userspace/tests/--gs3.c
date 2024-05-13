/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "sched.h"


#define THREADS3 50
#define PAGE_SIZE3 4096
#define STACK_AMOUNT3 5   // make sure this alligns with the define in UserSpaceMemoryManager.h
#define SUCCESS3 69

int all_threads_created3 = 0;

size_t functionpc3_1()
{
  size_t valid_array_size = (STACK_AMOUNT3 - 1) * PAGE_SIZE3;   // since array dont start right from top of page, 5 pages stack can only hold 4 pages long array
  char stack_data[valid_array_size];
  for (int i = 0; i < valid_array_size; i++)
  {
    stack_data[i] = 'A';
  }
  

  for (int i = (valid_array_size - 1); i >= 0; i--)
  {
    if (stack_data[i] != 'A')
    {
      // printf("debuging: failed at i = %d\n", i);
      return (size_t) -1;
    }
  }
  return (size_t) SUCCESS3;
}


size_t functionpc3()
{
  while(!all_threads_created3){sched_yield();}
  pthread_t thread_id;
  int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpc3_1, NULL);
  assert(rv == 0);

  size_t retval;
  rv = pthread_join(thread_id, (void**)&retval);
  assert(rv == 0);
  if (retval == (size_t) SUCCESS3)
  {
    return SUCCESS3;
  }
  return (size_t) -1;
}


//Test: 100 threads are created at the same time and grow at the same time
int main()
{
  pthread_t thread_id[THREADS3];
  
  // create 100 threads
  for(int i = 0; i < THREADS3; i++)
  {
      size_t rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))functionpc3, NULL);
      assert(rv == 0);
  }

  for (size_t i = 0; i < 200000000; i++)  // wait a bit for all thread to be fully initialzied
  {
    /* code */
  }
  
  all_threads_created3 = 1;
  // all threads should now call pthread_create at the same time

  // joining all threads and checking return value
  for(int i = 0; i < THREADS3; i++)
  {
    size_t retval;
    int rv = pthread_join(thread_id[i], (void**)&retval);
    assert(rv == 0);
    if ( retval != SUCCESS3)
    {
      return -1;
    }
  }

  return 0;
}

