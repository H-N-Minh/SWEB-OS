/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 666]
disabled: false
*/

#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"

#define PAGE_SIZE2 4096
#define STACK_AMOUNT2 5   // make sure this alligns with the define in UserSpaceMemoryManager.h


void* growingFoward2()
{
  size_t valid_array_size = (STACK_AMOUNT2 - 1) * PAGE_SIZE2;   // since array dont start right from top of page, 5 pages stack can only hold 4 pages long array
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
  return (void*) 0;
}

void* failingFoward2()
{
  size_t invalid_array_size = (STACK_AMOUNT2) * PAGE_SIZE2;   // since array dont start right from top of page, 5 pages stack can not hold 5 pages long array
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



// An array of 4 pages should fit into stack size of 5 pages, but an array of 5 pages should not fit and crashes.
// This is because array doesnt start right from the top of the page, so 4 pages long array are stored accross 5 pages
int main()
{
  pthread_t thread1, thread2;
  
  // Testing valid array, this shouldnt crash
  if (pthread_create(&thread1, NULL, growingFoward2, NULL) != 0)
  {
    printf("Failed to create thread 1\n");
    return -1;
  }
  int retval1 = -1;
  if (pthread_join(thread1, (void**) &retval1) != 0)
  {
    printf("Failed to join thread 1\n");
    return -1;
  }
  if (retval1 != 0)
  {
    printf("growingFoward2() failed\n");
    return -1;
  }

  // Testing invalid array, this should crash
  pid_t pid = fork();
  if (pid == 0)
  {
    if (pthread_create(&thread2, NULL, failingFoward2, NULL) != 0)
    {
      printf("Failed to create thread 2\n");
      exit(-3);
    }
    int retval2 = -1;
    if (pthread_join(thread2, (void**) &retval2) != 0)
    {
      printf("Failed to join thread 2\n");
      exit(-3);
    }
    
    exit(0);
  }
  else
  {
    int status;
    waitpid(pid, &status, 0);
    if (status == -3)
    {
      printf("Child process failed as expected, but for the wrong reason\n");
      return -1;
    }    
    else if (status != 0)
    {
      // printf("Child process crashed with exit code %d\n", status);
      // printf("Child process crashed, which is expected \n");
      return 0;
    }
  }
  printf("Child process did not crash, which is unexpected\n");
  return -1;  // this shouldnt be reached
}
