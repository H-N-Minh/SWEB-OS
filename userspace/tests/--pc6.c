/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

#define NUM_THREADS 250

int all_threads_created = 0;
int check_in [NUM_THREADS] = {0};

int function_pc6(int* arg)
{
  check_in[*arg] = 1;
  while(!all_threads_created){sched_yield();}
    int i = 4; 
    i++;
    assert(i == 5);
    return i;
}

//running 250 threads in paralell
int main()
{
    pthread_t thread_id[NUM_THREADS];

    int args[NUM_THREADS];
    for (size_t i = 0; i < NUM_THREADS; i++)
    {
      args[i] = i;
    }
    

    for(int i = 0; i < NUM_THREADS; i++)
    {
        int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function_pc6, (void*) (args + i));
        assert(rv == 0);
    }

    for (size_t i = 0; i < 200000000; i++)  // wait for all threads to be created
    {
      /* code */
    }

    for (size_t i = 0; i < NUM_THREADS; i++)
    {
      if (check_in[i] == 0)
      {
        printf("Thread %zu did not run\n", i);
        return -1;
      }
    }
    
    
    all_threads_created = 1;

    for(int i = 0; i < NUM_THREADS; i++)
    {
      void* retval;
      int rv = pthread_join(thread_id[i], &retval);
      assert(rv == 0);
      assert(retval = (void*)5);
    }    


    return 0;
}