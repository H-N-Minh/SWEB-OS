#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"
#include "wait.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

#define NUM_THREADS 20     // 20 threads, which fork into 20 processes

#define FAILURE9 666

int bruv6 = 0;
int all_threads_created9 = 0;


size_t threadFunction6()
{
    while(!all_threads_created9){sched_yield();}
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        bruv6 += 69;
        assert(bruv6 == 69 && "fork6_global_var should be 69 in every child process");
        exit(CHILD_SUCCESS);
    } 
    else if (pid > 0) 
    {
        int copy = bruv6;
        copy += 420;
        assert(copy == 420 && "fork6_global_var should be 420 in every threads of parent process");
        int status;
        waitpid(pid, &status, 0);
        if (status == CHILD_SUCCESS)
        {
            return CHILD_SUCCESS;
        }
    }
    return FAILURE9;
}



//Test: 100 threads calling 100 fork at the same time
int fork6() 
{
    pthread_t thread_id[NUM_THREADS];

    // create 100 threads
    for(int i = 0; i < NUM_THREADS; i++)
    {
        size_t rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))threadFunction6, NULL);
        assert(rv == 0);
    }

    for (size_t i = 0; i < 200000000; i++)  // wait a bit for all thread to be fully initialzied
    {
      /* code */
    }
    
    all_threads_created9 = 1;
    // all threads should now call fork at the same time

    // joining all threads and checking return value
    for(int i = 0; i < NUM_THREADS; i++)
    {
      size_t retval;
      int rv = pthread_join(thread_id[i], (void**)&retval);
      assert(rv == 0);
      if ( retval != CHILD_SUCCESS)
      {
        return -1;
      }
    }

    return PARENT_SUCCESS;
}


