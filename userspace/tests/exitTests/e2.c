#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

#define NUM_THREADS 100

int flag_exit = 0;

void function_exit()
{    
    while(!flag_exit){sched_yield();}
    exit(0);
}

//100 times exit at the same time
void e2_1()
{
    pthread_t thread_id[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++)
    {
        int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function_exit, NULL);
        assert(rv == 0);
    }
    flag_exit = 1;

    for(int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(thread_id[i], NULL);
        assert(0);
    }
    assert(0);

}

int e2()
{
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    e2_1();
    assert(0);    //this should never be reached

    return pid;
  } 
  else //parent
  {
    return 0;
  }
}
