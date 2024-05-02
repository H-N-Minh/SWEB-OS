#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <types.h>
#include <wait.h>
#include <pthread.h>
#include "assert.h"

#define NUM_CHILDREN 5

int shared_variable = 0;


void child_process(void* arg)
{
  sleep(1);
  pthread_exit((void*)4);
}

int waitpid8() {
  pid_t pid;
  int child_ids[NUM_CHILDREN];
  for (int i = 0; i < NUM_CHILDREN; ++i)
  {
    pid = fork();
    if (pid < 0)
    {
      printf("Fork failed\n");
      return 1;
    }
    else if (pid == 0)
    {
      child_process(&i);
    }
    else
    {
      child_ids[i] = pid;
    }
  }

  int status;
  for (int i = 0; i < NUM_CHILDREN; ++i)
  {
    int rv = waitpid(child_ids[i], &status, 0);
    assert(child_ids[i] == rv);
    assert(4 == status);
  }

  return 0;
}