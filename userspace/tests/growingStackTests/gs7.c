#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "wait.h"
#include "sched.h"

#define PAGE_SIZE7 4096
#define STACK_AMOUNT7 5        // make sure this alligns with the define in UserSpaceMemoryManager.h

char* global_array7 = 0;
int flag7 = 0;

void* thread_func7_1(void* arg)
{
  size_t valid_array_size = (STACK_AMOUNT7 - 1) * PAGE_SIZE7;   // since array dont start right from top of page, 5 pages stack can only hold 4 pages long array
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
      return (void*) -1;
    }
  }
  global_array7 = stack_data;
  return (void*) 0;
}

void* thread_func7_2(void* tid)
{
  pid_t pid = fork();
  if (pid == 0)
  {
    while (flag7 == 0)
    {
      sched_yield();
    }
    assert(global_array7 != 0);

    size_t address_to_access = ((size_t) global_array7) + PAGE_SIZE7 * (*(int*) tid);
    // printf("Thread %d trying to access address %zu\n", *(int*) tid, address_to_access - ((size_t) global_array7));
    *(char*) address_to_access = 'B';   // this should crash
    exit(0);
  }
  else
  {
    int retval;
    waitpid(pid, &retval, 0);
    if (retval == 0)
    {
      return (void*) 0;
    }
    return (void*) ((size_t) retval);
    
  }
  return (void*) 0;
}

// test if all pages are freed after thread dies
int gs7()
{
  pthread_t threads[STACK_AMOUNT7 + 1];
  int thread_ids[STACK_AMOUNT7];

  // create thread 1 and let it grow its stack
  pthread_create(&threads[0], NULL, thread_func7_1, NULL);
  pthread_join(threads[0], NULL);
  flag7 = 1;

  for (int i = 0; i < STACK_AMOUNT7; i++)
  {
    thread_ids[i] = i;
    pthread_create(&threads[i + 1], NULL, thread_func7_2, (void*) &(thread_ids[i]));
  }

  int reval [STACK_AMOUNT7];
  for (int i = 0; i < STACK_AMOUNT7; i++)
  {
    pthread_join(threads[i + 1], (void*) &reval[i]);
  }

  // if any thread succeeded, then the test failed. All threads should crash and fails
  for (int i = 0; i < STACK_AMOUNT7; i++)
  {
    if (reval[i] == 0)
    {
      printf("Thread %d didnt crash as it should\n", i + 1);
      return -1;
    }    
    // printf("Thread %d crashed as expected with code %d\n", i + 1, reval[i]);
  }

  return 0;
}
