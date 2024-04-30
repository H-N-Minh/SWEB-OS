#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "sched.h"


size_t SUCESS10 = 4251;
size_t FAIL10 = 666;

int* flag2_addr = 0;
int* local_addr = 0;

void* thread_func10_1(void* arg)
{
  int local_var = 45;
  assert(local_var == 45);

  while (flag2_addr == 0)
  {
    sched_yield();
  }
  assert(local_addr != 0);
  
  *local_addr = (*local_addr) * 10;
  *flag2_addr = 1;

  if (local_var == 45)
  {
    return (void*) SUCESS10;
  }

  return (void*) FAIL10;
}

void* thread_func10_2(void* arg)
{
  int local_var = 56;
  assert(local_var == 56 );

  int flag2 = 0;

  local_addr = &local_var;
  flag2_addr = &flag2;
  
  while (flag2 != 1)
  {
    sched_yield();
  }
  
  if (local_var == 560)
  {
    return (void*) SUCESS10;
  }
  
  return (void*) FAIL10;
}

// create 2 threads with 2 local variables. Despite having same name, they should still have different values
// Also, one thread can access the other thread's stack as well
int pc10()
{
  pthread_t thread1, thread2;
  int rv1, rv2;
  
  rv1 = pthread_create(&thread1, NULL, thread_func10_1, NULL);
  assert(rv1 == 0);
  
  rv2 = pthread_create(&thread2, NULL, thread_func10_2, NULL);
  assert(rv2 == 0);
  
  // Wait for both threads to finish
  void* retval1;
  void* retval2;
  pthread_join(thread1, &retval1);
  pthread_join(thread2, &retval2);
  
  if ((size_t)retval1 == SUCESS10 && (size_t)retval2 == SUCESS10)
  {
    return 0;
  }

  return -1;
}
