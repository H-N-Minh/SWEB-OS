#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"

#define MEGABYTE 1048576
#define PAGESIZE 4096

#define N 5
#define N_MEGABYTE N * MEGABYTE


#define ELEMENTS_IN_ARRAY N_MEGABYTE / 8
#define PAGES_IN_ARRAY N_MEGABYTE/PAGESIZE

#define THREAD_NUM5 64    // should be multiple of 2, so the array can be evenly divided between threads

size_t big_array5[ELEMENTS_IN_ARRAY];  //5 Megabytes

void* thread_function5(void* arg)
{
  int thread_id = *(int*)arg;
  int start = thread_id * (PAGES_IN_ARRAY / THREAD_NUM5);
  int end = (thread_id + 1) * (PAGES_IN_ARRAY / THREAD_NUM5);

  for(int i = start; i < end; i++)
  {
    big_array5[i * (PAGESIZE / 8)] = (size_t)i;
  }
  for(int i = start; i < end; i++)
  {
    assert((big_array5[i * (PAGESIZE / 8)]) == (size_t)i);
  }
  // printf("Thread %d finished\n", thread_id);
  return NULL;
}

int pra5()
{
  setPRA(__NFU_PRA__); 
  int hit;
  int miss;
  getPRAstats(&hit, &miss);
  printf("NFU PRA: Hit: %d, Miss: %d (before test)\n", hit, miss);

  pthread_t threads[THREAD_NUM5];
  int thread_ids[THREAD_NUM5];

  for(int i = 0; i < THREAD_NUM5; i++)
  {
    thread_ids[i] = i;
    pthread_create(&threads[i], NULL, thread_function5, &thread_ids[i]);
  }
  
  // printf("Threads created, waiting for them to finish\n");
  for(int i = 0; i < THREAD_NUM5; i++)
  {
    pthread_join(threads[i], NULL);
  }

  // printf("all Threads finished\n");

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    if (big_array5[i * (PAGESIZE / 8)] != i)
    {
      printf("Error: big_array5[%ld] != i: %d\n", big_array5[i * (PAGESIZE / 8)], i);
    }
    assert(big_array5[i * (PAGESIZE / 8)] == i);
  }

  getPRAstats(&hit, &miss);
  printf("NFU PRA: Hit: %d, Miss: %d (after test)\n", hit, miss);

  return 0;
}