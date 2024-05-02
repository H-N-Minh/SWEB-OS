#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 5
#define NUM_ITERATIONS 10

sem_t empty, full;
pthread_mutex_t buffer_lock;

int buffer[BUFFER_SIZE];
int in = 0, out = 0;

void producer()
{
  int item = 0;
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {
    //produce item
    sleep(1);
    item++;

    sem_wait(&empty);
    pthread_mutex_lock(&buffer_lock);

    buffer[in] = item;
    printf("Produced item %d at position %d\n", item, in);
    in = (in + 1) % BUFFER_SIZE;

    pthread_mutex_unlock(&buffer_lock);
    sem_post(&full);
  }
  pthread_exit(NULL);
}

void consumer()
{
  int item;
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {
    sem_wait(&full);
    pthread_mutex_lock(&buffer_lock);

    item = buffer[out];
    printf("Consumed item %d from position %d\n", item, out);
    out = (out + 1) % BUFFER_SIZE;

    pthread_mutex_unlock(&buffer_lock);
    sem_post(&empty);

    sleep(2);
  }
  pthread_exit(NULL);
}

int sem9()
{
  pthread_t producer_thread, consumer_thread;

  sem_init(&empty, 0, BUFFER_SIZE - 1);
  sem_init(&full, 0, 0);
  pthread_mutex_init(&buffer_lock, NULL);

  pthread_create(&producer_thread, NULL, (void * (*)(void *))producer, NULL);
  pthread_create(&consumer_thread, NULL, (void * (*)(void *))consumer, NULL);

  pthread_join(producer_thread, NULL);
  pthread_join(consumer_thread, NULL);

  sem_destroy(&empty);
  sem_destroy(&full);
  pthread_mutex_destroy(&buffer_lock);

  return 0;
}
