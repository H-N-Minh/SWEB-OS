#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "assert.h"

#define THREAD_COUNT 4
#define NUM_ITERATIONS 100

pthread_mutex_t mutexM;
pthread_spinlock_t spinlockM;
sem_t semaphoreM;
pthread_cond_t conditionM;
int shared_data = 0;

void spinlock_thread(void* arg)
{
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {
    pthread_spin_lock(&spinlockM);
    shared_data++;
    pthread_spin_unlock(&spinlockM);
  }
  printf("Thread with spinlock incremented shared_data to %d\n", shared_data);
}

void mutex_thread(void* arg)
{
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {
    pthread_mutex_lock(&mutexM);
    shared_data++;
    pthread_mutex_unlock(&mutexM);
  }
  printf("Thread with mutex incremented shared_data to %d\n", shared_data);
}

void semaphore_thread(void* arg)
{
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {
    sem_wait(&semaphoreM);
    shared_data++;
    sem_post(&semaphoreM);
  }
  printf("Thread with semaphore incremented shared_data to %d\n", shared_data);
}

void condition_thread(void* arg)
{
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {
    pthread_mutex_lock(&mutexM);
    while (shared_data < 300)
    {
      pthread_cond_wait(&conditionM, &mutexM);
    }
    assert(shared_data >= 300);
    shared_data++;
    pthread_mutex_unlock(&mutexM);
  }
  printf("Thread with condition variable incremented shared_data to %d\n", shared_data);
}

int mixLocking()
{
  int rv;
  pthread_t threads[THREAD_COUNT];

  rv = pthread_spin_init(&spinlockM, 0);
  assert(rv == 0);

  rv = pthread_mutex_init(&mutexM, NULL);
  assert(rv == 0);

  sem_init(&semaphoreM, 0, 1);
  rv = pthread_cond_init(&conditionM, NULL);

  rv = pthread_create(&threads[0], NULL, (void* (*)(void*))spinlock_thread, NULL);
  assert(rv == 0);

  rv = pthread_create(&threads[1], NULL, (void* (*)(void*))mutex_thread, NULL);
  assert(rv == 0);

  rv = pthread_create(&threads[2], NULL, (void* (*)(void*))semaphore_thread, NULL);
  assert(rv == 0);

  rv = pthread_create(&threads[3], NULL, (void* (*)(void*))condition_thread, NULL);
  assert(rv == 0);

  for (int i = 0; i < THREAD_COUNT; i++)
  {
    pthread_join(threads[i], NULL);
  }

  rv = pthread_spin_destroy(&spinlockM);
  assert(rv == 0);

  rv = pthread_mutex_destroy(&mutexM);
  assert(rv == 0);

  rv = sem_destroy(&semaphoreM);
  assert(rv == 0);

  rv = pthread_cond_destroy(&conditionM);
  assert(rv == 0);

  return 0;

}