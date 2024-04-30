#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

#define NUM_THREADS 250

int all_threads_created = 0;

int function_pc6()
{
  while(!all_threads_created){sched_yield();}
    int i = 4; 
    i++;
    assert(i == 5);
    return i;
}

//running 250 threads in paralell
int pc6()
{
    pthread_t thread_id[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++)
    {
        int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function_pc6, NULL);
        assert(rv == 0);
    }

    for (size_t i = 0; i < 200000000; i++)  // wait for all threads to be created
    {
      /* code */
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