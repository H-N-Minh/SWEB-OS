#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <types.h>
#include <wait.h>
#include <pthread.h>

#define NUM_CHILDREN 5

int shared_variable = 0;
pthread_mutex_t lock;

void child_process(void* arg)
{
  int child_id = *(int*)arg;
  pthread_mutex_lock(&lock);
  printf("Child %d: Shared variable before increment: %d\n", child_id, shared_variable);
  shared_variable++;
  printf("Child %d: Shared variable after increment: %d\n", child_id, shared_variable);
  pthread_mutex_unlock(&lock);
}

int main() {
  pthread_mutex_init(&lock, NULL);

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
      exit(0);
    }
    else
    {
      child_ids[i] = pid;
    }
  }

  int status;
  for (int i = 0; i < NUM_CHILDREN; ++i)
  {
    waitpid(child_ids[i], &status, 0);
  }

  pthread_mutex_destroy(&lock); // Destroy the mutex lock
  return 0;
}