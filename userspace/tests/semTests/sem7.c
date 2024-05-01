#include <stdio.h>
#include <pthread.h>
#include "semaphore.h"

#define NUM_THREADS 10
#define ITERATIONS 1000

sem_t semaphore;
int shared_counter = 0;

void increment_counter7()
{
  for (int i = 0; i < ITERATIONS; ++i)
  {
    sem_wait(&semaphore);
    shared_counter++;
    sem_post(&semaphore);
  }
}

void decrement_counter7()
{
  for (int i = 0; i < ITERATIONS; ++i)
  {
    sem_wait(&semaphore);
    shared_counter--;
    sem_post(&semaphore);
  }
}

int sem7()
{
  pthread_t threads[NUM_THREADS];

  sem_init(&semaphore, 0, 1);

  for (int i = 0; i < NUM_THREADS / 2; ++i)
  {
    pthread_create(&threads[i], NULL, (void* (*)(void*))increment_counter7, NULL);
  }

  for (int i = NUM_THREADS / 2; i < NUM_THREADS; ++i)
  {
    pthread_create(&threads[i], NULL, (void* (*)(void*))decrement_counter7, NULL);
  }

  for (int i = 0; i < NUM_THREADS; ++i)
  {
    pthread_join(threads[i], NULL);
  }

  printf("Final value of shared_counter: %d\n", shared_counter);

  sem_destroy(&semaphore);
  if(shared_counter == 0)
    return 0;
  else
    return -1;
}
