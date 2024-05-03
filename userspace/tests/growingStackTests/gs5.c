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
char* local5_addr = 0;


void growStack(char c, char* stack_data)
{
  assert(stack_data != 0);
  for (int i = 0; i < VALID_ARRAY_SIZE5; i++)
  {
    stack_data[i] = c; 
  }
}

int checkStack(char c, char* stack_data)
{
  assert(stack_data != 0);
  for (int i = 0; i < VALID_ARRAY_SIZE5; i++)
  {
    if (stack_data[i] != c)
    {
      return 0;
    }
  }
  return 1;
}

void* thread_func5_1(void* arg)
{

  char array_adr[VALID_ARRAY_SIZE5];
  growStack('A', array_adr);
  assert(checkStack('A', array_adr));
  int local_var = 45;
  assert(local_var == 45);

  // step 1

  while (flag5 == 0)
  {
    // step 2
    sched_yield();
  }
  // step 5
  assert(local5_addr != 0);
  
  growStack('Z', local5_addr);
  assert(checkStack('Z', local5_addr));
  flag5 = 0;

  while (flag5 == 0)
  {
    // step 6
    sched_yield();
  }

  // step 9
  assert(local_var == 45);
  assert(checkStack('A', array_adr));

  growStack('Z', local5_addr);  // this should crash because thread 2 died
  assert(checkStack('Z', local5_addr));

  return (void*) SUCESS5;
}

void* thread_func5_2(void* arg)
{
  char array_adr[VALID_ARRAY_SIZE5];
  growStack('B', array_adr);
  assert(checkStack('B', array_adr));
  // step 3

  local5_addr = array_adr;
  flag5 = 1;
  
  while (flag5 == 1)
  {
    // step 4
    sched_yield();
  }
  
  // step 7
  if (checkStack('Z', array_adr))   // thread 1 should have changed our local array to Z
  {
    return (void*) SUCESS5;
  }
  
  return (void*) FAIL5;
}

// 2 threads have different stack but they should still be able to access each other's stack
int child_func5()
{
  pthread_t thread1, thread2;
  int rv1, rv2;
  
  rv1 = pthread_create(&thread1, NULL, thread_func5_1, NULL);
  assert(rv1 == 0);
  
  rv2 = pthread_create(&thread2, NULL, thread_func5_2, NULL);
  assert(rv2 == 0);
  
  // wait for thread 2 to successfully finish
  void* retval2;
  pthread_join(thread2, &retval2);
  if ((size_t) retval2 == FAIL5)
  {
    exit(-3);
  }
  
  // signal thread 1 to continue
  // step 8
  flag5 = 1;

  // wait for thread 1 to crash
  void* retval1;
  pthread_join(thread1, &retval1);
  
  if ((size_t)retval1 == FAIL5)
  {
    exit(-3);    // this shouldnt be reached
  }

  exit(0);   // this shouldnt be reached
  return 0;
}


// both thread has its own big array, thread 1 should be able to access thread 2's array and rewrite it
// then kill 2nd thread and thread 1 try to access the stack again, this should be invalid and crash
int gs5()
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
