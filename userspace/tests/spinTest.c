#include "stdio.h"
#include "pthread.h"

#define NUM_THREADS 100
#define MAX_COUNT   1000

int counter = 0;
pthread_spinlock_t lock;

void increment_counter()
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        pthread_spin_lock(&lock);
        counter++;
        pthread_spin_unlock(&lock);
    }
    printf("Counter value: %d\n", counter);
}

int main() {
  pthread_t threads[NUM_THREADS];
  pthread_spin_init(&lock, 0);

  for (int t = 0; t < NUM_THREADS; t++)
  {
      pthread_create(&threads[t], NULL, (void* (*)(void*))increment_counter, NULL);
  }

  for (int t = 0; t < NUM_THREADS; t++)
  {
    pthread_join(threads[t], NULL);
  }


  printf("Final Counter value: %d\n", counter);

  return 0;
}