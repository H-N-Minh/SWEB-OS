#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "wait.h"
#include "sched.h"

#define PAGE_SIZE6 4096

int* local_addr6 = 0;



void thread_func6_1()
{
  while (local_addr6 == 0)
  {
    sched_yield();
  }
  // step 2
  
  *local_addr6 = (*local_addr6) * 10;   // this should crash

}

void thread_func6_2()
{
  // step 1
  int local_var = 42;
  size_t unmapped_page = ((size_t) &local_var) - PAGE_SIZE6;
  local_addr6 = (int*) unmapped_page;

  // make sure thread stay alive
  while (1)
  {
    /* code */
  }
}

// controller for 2 threads
int child_func6()
{
  pthread_t thread1, thread2;
  int rv1, rv2;
  
  rv1 = pthread_create(&thread1, NULL, (void * (*)(void *)) thread_func6_1, NULL);
  assert(rv1 == 0);
  
  rv2 = pthread_create(&thread2, NULL, (void * (*)(void *)) thread_func6_2, NULL);
  assert(rv2 == 0);
  

  // wait for thread 1 to crash
  void* retval1;
  pthread_join(thread1, &retval1);

  exit(0);   // this shouldnt be reached
  return 0;
}


// Testing gs6: trying to access another thread's unmapped page even when that thread still alive
int gs6()
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
