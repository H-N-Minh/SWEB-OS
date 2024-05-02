#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "wait.h"
#include "sched.h"

#define PAGE_SIZE8 4096
#define STACK_AMOUNT8 5   // make sure this alligns with the define in UserSpaceMemoryManager.h


int thread_2_done_flag = 0;


void thread_func6_1()
{
  while (thread_2_done_flag == 0)
  {
    sched_yield();
  }

  size_t invalid_array_size = STACK_AMOUNT8 * PAGE_SIZE8;   // since array dont start right from top of page, 5 pages stack can not hold 5 pages long array => overflow to next thread
  char stack_data[invalid_array_size];
  for (int i = 0; i < invalid_array_size; i++)
  {
    stack_data[i] = 'A';
  }
  

  for (int i = (invalid_array_size - 1); i >= 0; i--)
  {
    if (stack_data[i] != 'A')
    {
      // printf("debuging: failed at i = %d\n", i);
      return (void*) -1;
    }
  }
  // even tho the array size is invalid, thread 1 should not crash and exit with success
  thread_2_done_flag = 0;  // signal thread 2 that thread 1 finished overflowing
  return (void*) 0;
}

void thread_func8_2()
{
  thread_2_done_flag = 1;
  while (thread_2_done_flag == 1)
  {
    sched_yield();
  }

  size_t invalid_array_size = STACK_AMOUNT8 * PAGE_SIZE8;   // since array dont start right from top of page, 5 pages stack can not hold 5 pages long array => overflow to next thread
  char stack_data[invalid_array_size];
  for (int i = 0; i < invalid_array_size; i++)
  {
    stack_data[i] = 'A';
  }
  

  for (int i = (invalid_array_size - 1); i >= 0; i--)
  {
    if (stack_data[i] != 'A')
    {
      // printf("debuging: failed at i = %d\n", i);
      return (void*) -1;
    }
  }
  return (void*) 0;

}

// controller for 2 threads
int child_func6()
{
  pthread_t thread1, thread2;
  int rv1, rv2;
  
  rv1 = pthread_create(&thread1, NULL, (void * (*)(void *)) thread_func8_1, NULL);
  assert(rv1 == 0);
  
  rv2 = pthread_create(&thread2, NULL, (void * (*)(void *)) thread_func8_2, NULL);
  assert(rv2 == 0);
  

  // wait for thread 1 to crash
  void* retval1;
  pthread_join(thread1, &retval1);

  exit(0);   // this shouldnt be reached
  return 0;
}


// test overflow detection: 1 thread gets a really big array that it overflows to the next threads stack
int gs8()
{
  pid_t pid = fork();
  if (pid == 0)
  {
    child_func6();
    exit(0);
  }
  else
  {
    int status;
    waitpid(pid, &status, 0);
    if (status == 0)
    {
      printf("Child process didnt crash as it should\n");
      return -1;
    }
    // printf("Child process crashed as expected with code %d\n", status);
  }
  
  return 0;
}
