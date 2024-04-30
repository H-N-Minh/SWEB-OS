#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "sched.h"


#define THREADS9 10


int all_threads_created9 = 0;

size_t functionpc9_1(size_t simple_argument)
{
  size_t retval = *(size_t*) simple_argument + 1;
  return retval;
}


size_t functionpc9(size_t simple_argument)
{
  // printf("simple_argument: %zu\n", simple_argument);
  while(!all_threads_created9){sched_yield();}
  pthread_t thread_id;
  int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpc9_1, (void*) simple_argument);
  assert(rv == 0);

  void* value_ptr; 
  rv = pthread_join(thread_id, &value_ptr);
  assert(rv == 0);
  assert((size_t)value_ptr == (*(size_t*)simple_argument + 1)  && "threads dont have unique parameters or return values"); 
  // printf("value_ptr: %zu\n", (size_t) value_ptr + 1);
  return ((size_t) value_ptr) + 1;
}


//Test: 100 threads calling 100 pcreate at the same time
// also test that alll threads should have different args and differnt return value 
int pc9()
{
    pthread_t thread_id[THREADS9];
    size_t thread_arg[THREADS9];

    // fill arg
    for (size_t i = 0; i < THREADS9; i++)
    {
      thread_arg[i] = i;  
    }
    // create 100 threads
    for(int i = 0; i < THREADS9; i++)
    {
        size_t rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))functionpc9, (void*) (thread_arg + i));
        assert(rv == 0);
    }

    for (size_t i = 0; i < 200000000; i++)  // wait a bit for all thread to be fully initialzied
    {
      /* code */
    }
    
    all_threads_created9 = 1;
    // all threads should now call pthread_create at the same time

    // joining all threads and checking return value
    for(int i = 0; i < THREADS9; i++)
    {
      size_t retval;
      int rv = pthread_join(thread_id[i], (void**)&retval);
      assert(rv == 0);
      if ( retval != (thread_arg[i] + 2))
      {
        printf("PC9 Error!: threads dont have unique parameters or return values\n");
        return -1;
      }
    }

    return 0;
}

