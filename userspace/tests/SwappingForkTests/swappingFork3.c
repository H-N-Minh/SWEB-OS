#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"
#include "wait.h"


#define NUM_THREADS 20

int global = 0;
int all_threads_created = 0;


size_t ipt3_function()
{
    while(!all_threads_created){sched_yield();}
    
    pid_t pid = fork();
    if (pid == 0) 
    {
      global += 69;
      assert(global == 69 && "wrong global value in child");
      exit(1);
    } 
    else if (pid > 0) 
    {
      int copy = global;
      copy += 420;
      assert(copy == 420 && "fork6_global_var should be 420 in every threads of parent process");
      int status;
      waitpid(pid, &status, 0);
      if (status != 1)
      {
        assert(0 && "Child exits with wrong value.\n");
      }
      return 3;
    }
    else
    {
      assert(0 && "Fork failed");
    }
    return -1;
}


pthread_t thread_id[NUM_THREADS];
//Test: Testing fork after swapping (this test only makes sense if we run the previous test as well)
int swappingFork3() 
{
    for(int i = 0; i < NUM_THREADS; i++)
    {
        size_t rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))ipt3_function, NULL);
        assert(rv == 0);
    }
    
    all_threads_created = 1;
    
    for(int i = 0; i < NUM_THREADS; i++)
    {
      size_t retval;
      int rv = pthread_join(thread_id[i], (void**)&retval);
      assert(rv == 0);
      if (retval != 3)
      {
        assert(0 && "Function failed");
      }
    }
    return 0;
}


