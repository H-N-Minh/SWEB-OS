#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

int all_threads_created = 0;

int function_pc6()
{
  while(!all_threads_created){sched_yield();}
    int i = 4; 
    i++;
    assert(i == 5);
    return i;
}

//starting 250 threads
int pc6()
{
    pthread_t thread_id[250];

    for(int i = 0; i < 250; i++)
    {
        int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function_pc6, NULL);
        assert(rv == 0);
    }
    all_threads_created = 1;

    for(int i = 0; i < 250; i++)
    {
      void* retval;
      int rv = pthread_join(thread_id[i], &retval);
      assert(rv == 0);
      assert(retval = (void*)5);
    }
    
    printf("pc6 successful!\n");
    
    return 0;
}