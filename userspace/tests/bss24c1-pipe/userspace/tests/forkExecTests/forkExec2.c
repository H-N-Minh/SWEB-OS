#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "pthread.h"
#include "types.h"

#define NUMBER_OF_THREADS 25

int flag = 0;

int fork_exec_function(int tid)
{
  while(!flag){}
  pid_t pid = fork();

  if (pid < 0) 
  {
    return -1;
  } 

  else if (pid == 0)
  {
    const char * path = "usr/forkExec2_testprogram.sweb";

    if(tid == 0 || tid == 2  || tid == 3 )
    {
      char *argv[] = {"...", (char *)0 };
      execv(path, argv);
    }

    else
    {
      char *argv[] = {"_", (char *)0 };
      execv(path, argv);
    }
    

    
    assert(0);

  } 
  else
  {
    return 0;
  }
  return -1;
}


int forkExec2()
{
  int counter = 0;
  pthread_t threads_ids[NUMBER_OF_THREADS];
  for(size_t i = 0; i < NUMBER_OF_THREADS; i++)
  {
    int rv = pthread_create(&threads_ids[i], NULL, (void*)fork_exec_function, (void*)i);
    assert(rv == 0);
  }
  sleep(1);
  flag = 1;
  void* rvs[NUMBER_OF_THREADS];
  for(int i = 0; i < NUMBER_OF_THREADS; i++)
  {
    int rv = pthread_join(threads_ids[i], &rvs[i]);
    assert(rvs[0] == (void*)0);
    assert(rv == 0);
    if(rvs[0] == (void*)0)
    {
      counter++;
    }
  }
  //printf("Counter %d", counter);
  assert(counter == NUMBER_OF_THREADS);

  return 0;
}