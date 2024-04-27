#include "stdio.h"
#include "stdlib.h"
#include "wait.h"
#include "unistd.h"
#include "assert.h"
#include "pthread.h"

#define NUM_THREADS 20


int function()
{  
  pid_t pid;
  int status;

  pid = fork();
  assert(pid >= 0);

  if (pid == 0) //Child
  {
    exit(3);
  }
  else //Parent
  {
    int rv = waitpid(pid, &status, 0);
    assert(rv == pid);
    assert(status == 3);
  }  
  return 7;
}

//Waitpid with pthread_create
int waitpid6()
{


  pthread_t thread_id[NUM_THREADS];

  for(int i = 0; i < NUM_THREADS; i++)
  {
    int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function, NULL);
    assert(rv == 0);
  }

  for(int i = 0; i < NUM_THREADS; i++)
  {
    assert(thread_id[i] != 0);
    void* retval;
    int rv = pthread_join(thread_id[i], &retval);
    assert(retval == (void*)7);
  }
    
  return 0;
}