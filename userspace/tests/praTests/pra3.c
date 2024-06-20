#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"

#define PAGESIZE 4096
#define PAGES_IN_ARRAY 1280 
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE)/8

#define THREAD_NUM5 64    // should be multiple of 2, so the array can be evenly divided between threads

size_t big_array3[ELEMENTS_IN_ARRAY];  //5 Megabytes


pthread_t threads[THREAD_NUM5];
int thread_ids[THREAD_NUM5];


void* thread_function5(void* arg)
{
  int thread_id = *(int*)arg;
  int start = thread_id * (PAGES_IN_ARRAY / THREAD_NUM5);
  int end = (thread_id + 1) * (PAGES_IN_ARRAY / THREAD_NUM5);

  for(int i = start; i < end; i++)
  {
    big_array3[i * (PAGESIZE / 8)] = (size_t)i;
  }
  for(int i = start; i < end; i++)
  {
    assert((big_array3[i * (PAGESIZE / 8)]) == (size_t)i);
  }
  // printf("Thread %d finished\n", thread_id);
  return NULL;
}

//Test: testing 64 threads writing to array in parallel
int pra3()
{

  for(int i = 0; i < THREAD_NUM5; i++)
  {
    thread_ids[i] = i;
    pthread_create(&threads[i], NULL, thread_function5, &thread_ids[i]);
  }
  
  printf("Threads created, waiting for them to finish\n");
  for(int i = 0; i < THREAD_NUM5; i++)
  {
    pthread_join(threads[i], NULL);
  }

  printf("all Threads finished\n");

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    if (big_array3[i * (PAGESIZE / 8)] != i)
    {
      assert(0);
    }
    assert(big_array3[i * (PAGESIZE / 8)] == i);
  }
  return 0;
}