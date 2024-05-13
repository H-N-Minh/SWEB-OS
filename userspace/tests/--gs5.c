/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 666]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "wait.h"
#include "sched.h"

#define PAGE_SIZE5 4096
#define STACK_AMOUNT5 5   // make sure this alligns with the define in UserSpaceMemoryManager.h
#define VALID_ARRAY_SIZE5 (STACK_AMOUNT5 - 1) * PAGE_SIZE5    // 4 pages array should fit in 5 pages stack
size_t SUCESS5 = 4251;
size_t FAIL5 = 666;

int flag5 = 0;
char* global_addr5 = 0;     // used to store local array of thread 2

/**
 * write a given array with a given character
*/
void growStack(char c, char* stack_data)
{
  assert(stack_data != 0);
  for (int i = 0; i < VALID_ARRAY_SIZE5; i++)
  {
    stack_data[i] = c; 
  }
}

/**
 * check if the given array is filled with the given character
 * return 1 if it is, 0 if it is not
*/
int checkStack(char c, char* stack_data)
{
  assert(stack_data != 0);
  for (int i = 0; i < VALID_ARRAY_SIZE5; i++)
  {
    if (stack_data[i] != c)
    {
      // printf("given letter is %c, but found %c\n", c, stack_data[i]);
      return 0;
    }
  }
  return 1;
}

void* thread_func5_1(void* arg)
{

  char local_array[VALID_ARRAY_SIZE5];
  growStack('A', local_array);
  assert(checkStack('A', local_array));
  int local_var = 45;
  assert(local_var == 45);

  // step 1

  while (flag5 == 0)
  {
    // step 2
    sched_yield();
  }
  // printf("thread 1: step 1: thread 1 woke up, checking if the array of thread 2 is B\n");
  // step 5
  assert(global_addr5 != 0);
  
  if (!checkStack('B', global_addr5))   // thread 1 should have changed our local array to Z
  {
    // printf("thread 1: step 2.1: array of thread 2 is not B as expected, exiting with error\n");
    return (void*) FAIL5;
  }
  // printf("thread 1: step 2: array of t2 is all B, which is correct\n");
  growStack('Z', global_addr5);
  assert(checkStack('Z', global_addr5));
  // printf("thread 1: step 3: changed array of t2 to all Z, then wake thread 2 up\n");
  flag5 = 0;

  while (flag5 == 0)
  {
    // step 6
    sched_yield();
  }
  // printf("thread 1: step 7: t1 just woke up, checking if its local array still A\n");
  // step 9
  assert(local_var == 45);
  assert(checkStack('A', local_array));
  // printf("thread 1: step 8: all local var still unchanged, correct!. checking array of thread 2, now process should crash\n");
  assert(checkStack('Z', global_addr5));    // if this is commented out, process should not crash, so test case should shows failure
  // printf("thread 1: step 9: local array t2 is still accesible = ERROR\n");


  return (void*) SUCESS5;   // this should never reached
}

void* thread_func5_2(void* arg)
{
  char local_array[VALID_ARRAY_SIZE5];
  growStack('B', local_array);
  assert(checkStack('B', local_array));

  global_addr5 = local_array;
  // printf("thread 2: step 0: local array set to B, signalling thread 1 to wake up\n");
  flag5 = 1;
  while (flag5 == 1)
  {
    sched_yield();
  }
  // printf("thread 2: step 4: thread 2 woke up, checking if array is changed to Z by thread 1 successfully\n");
  
  if (checkStack('Z', local_array))   // thread 1 should have changed our local array to Z
  {
    // printf("thread 2: step 5.1, array of t2 is  Z correctly, t2 dies with success\n");
    return (void*) SUCESS5;
  }
  // printf("thread 2: step 5, array is not Z as expected, thread 2 exits with failure\n");
  return (void*) FAIL5;
}

// this is child process, it creates 2 threads. this process should crash
int child_func5()
{
  pthread_t thread1, thread2;
  int rv1, rv2;
  rv1 = pthread_create(&thread1, NULL, thread_func5_1, NULL);
  assert(rv1 == 0);
  rv2 = pthread_create(&thread2, NULL, thread_func5_2, NULL);
  assert(rv2 == 0);
  
  // wait for thread 2 to successfully finish with return 0
  void* retval2;
  pthread_join(thread2, &retval2);
  if ((size_t) retval2 == FAIL5)
  {
    exit(-3);
  }
  // printf("child:    step 6: t2 died with success, wake up t1 now and join it \n");
  // signal thread 1 to continue
  flag5 = 1;

  // wait for thread 1 to crash
  void* retval1;
  pthread_join(thread1, &retval1);
  
  // this shoouldnt be reached
  if ((size_t)retval1 == FAIL5)
  {
    exit(-3);  
  }
  exit(0); 
  return 0;
}


// both thread has its own big array, thread 1 array of 'A' and thread 2 array of 'B'
// thread 1 should be able to access thread 2's array and rewrite it completely to array of 'Z'
// then kill 2nd thread and thread 1 try to access the stack again, this should be invalid and crash
int main()
{
  pid_t pid = fork();
  if (pid == 0)
  {
    child_func5();
    exit(-3);
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
    else if (status == 0)
    {
      printf("Child process didnt crash as it should\n");
      return -1;
    }
    // printf("Child process crashed as expected with code %d\n", status);
  }
  
  return 0;
}
