#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "wait.h"
#include "sched.h"


size_t SUCESS5 = 4251;
size_t FAIL5 = 666;

int flag5 = 0;
int* local5_addr = 0;



void* thread_func5_1(void* arg)
{
  // step 1
  int local_var = 45;
  assert(local_var == 45);

  while (flag5 == 0)
  {
    // step 2
    sched_yield();
  }
  // step 5
  assert(local5_addr != 0);
  
  *local5_addr = (*local5_addr) * 10;
  flag5 = 0;

  while (flag5 == 0)
  {
    // step 6
    sched_yield();
  }

  // step 9
  assert(local_var == 45);

  *local5_addr = (*local5_addr) * 10;  // this should crash

  return (void*) SUCESS5;
}

void* thread_func5_2(void* arg)
{
  // step 3
  int local_var = 56;
  assert(local_var == 56 );

  local5_addr = &local_var;
  flag5 = 1;
  
  while (flag5 == 1)
  {
    // step 4
    sched_yield();
  }
  
  // step 7
  if (local_var == 560)
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
    exit(-1);
  }
  
  // signal thread 1 to continue
  // step 8
  flag5 = 1;

  // wait for thread 1 to crash
  void* retval1;
  pthread_join(thread1, &retval1);
  
  if ((size_t)retval1 == FAIL5)
  {
    exit(-1);    // this shouldnt be reached
  }

  exit(0);   // this shouldnt be reached
  return 0;
}


// first make sure 2 threads can access each other stack. this should be valid
// then kill 1 thread and try to access its stack again, this should be invalid and crash
int gs5()
{
  pid_t pid = fork();
  if (pid == 0)
  {
    child_func5();
    exit(-1);
  }
  else
  {
    int status;
    waitpid(pid, &status, 0);
    if (status == -1)
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
