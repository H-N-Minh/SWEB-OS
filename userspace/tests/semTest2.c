#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_THREADS 2
#define MAX_COUNT 1000000

int shared_var1 = 0;
int shared_var2 = 0;
sem_t semaphore1, semaphore2;

void test_function(void* arg)
{
  int thread_id = *((int*)arg);

  for (int i = 0; i < MAX_COUNT; i++)
  {
    if (thread_id == 0)//thread 1
    {
      sem_wait(&semaphore1);
      shared_var1++;
      sem_post(&semaphore2);
    }
    else//thread 2
    {
      sem_wait(&semaphore2);
      shared_var2++;
      sem_post(&semaphore1);
    }
  }
}

int main()
{
  pthread_t threads[NUM_THREADS];
  int thread_ids[NUM_THREADS] = {0, 1};

  sem_init(&semaphore1, 0, 1);
  sem_init(&semaphore2, 0, 0);

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_create(&threads[i], NULL, (void* (*)(void*))test_function, &thread_ids[i]);
  }

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_join(threads[i], NULL);
  }

  sem_destroy(&semaphore1);
  sem_destroy(&semaphore2);

  printf("Shared Variable 1: %d\n", shared_var1);
  printf("Shared Variable 2: %d\n", shared_var2);

  return 0;
}
