/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define NUM_ITERATIONS 10
#define NUM_CONSUMERS 3

pthread_mutex_t buffer_lock;
pthread_mutex_t producer_finished_lock;
pthread_cond_t cond_not_empty, cond_not_full;
pthread_mutex_t count_lock;
pthread_mutex_t producer_finished_lock;


int buffer[BUFFER_SIZE];
int in = 0, out = 0, count = 0;
int producer_finished = 0;

void producer()
{
  int item = 0;
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {
    sleep(1);
    item++;

    pthread_mutex_lock(&count_lock);
    while (count == BUFFER_SIZE)
    {
      pthread_cond_wait(&cond_not_full, &count_lock);
    }

    pthread_mutex_lock(&buffer_lock);
    buffer[in] = item;
    pthread_mutex_unlock(&buffer_lock);
    printf("Produced item %d at position %d\n", item, in);
    in = (in + 1) % BUFFER_SIZE;
    count++;

    pthread_cond_signal(&cond_not_empty);
    pthread_mutex_unlock(&count_lock);
  }

  pthread_mutex_lock(&producer_finished_lock);
  producer_finished = 1;
  pthread_mutex_unlock(&producer_finished_lock);

  pthread_cond_broadcast(&cond_not_empty);

}

void consumer()
{
  int item;
  while (1)
  {
    pthread_mutex_lock(&count_lock);

    while (count == 0 && !producer_finished)
    {
      pthread_cond_wait(&cond_not_empty, &count_lock);
    }

    if (producer_finished && count == 0)
    {
      pthread_mutex_unlock(&count_lock);
      break;
    }

    pthread_mutex_lock(&buffer_lock);
    item = buffer[out];
    pthread_mutex_unlock(&buffer_lock);
    printf("Consumed item %d from position %d\n", item, out);
    out = (out + 1) % BUFFER_SIZE;
    count--;

    pthread_cond_signal(&cond_not_full);
    pthread_mutex_unlock(&count_lock);

    sleep(2);
  }

}

int main()
{
  pthread_t producer_thread;
  pthread_t consumer_threads[NUM_CONSUMERS];

  pthread_mutex_init(&buffer_lock, NULL);
  pthread_mutex_init(&count_lock, NULL);
  pthread_mutex_init(&producer_finished_lock, NULL);


  pthread_cond_init(&cond_not_empty, NULL);
  pthread_cond_init(&cond_not_full, NULL);

  pthread_create(&producer_thread, NULL, (void * (*)(void *))producer, NULL);
  for (int i = 0; i < NUM_CONSUMERS; i++)
  {
    pthread_create(&consumer_threads[i], NULL, (void * (*)(void *))consumer, NULL);
  }

  pthread_join(producer_thread, NULL);
  for (int i = 0; i < NUM_CONSUMERS; i++)
  {
    pthread_join(consumer_threads[i], NULL);
  }

  pthread_mutex_destroy(&buffer_lock);
  pthread_mutex_destroy(&count_lock);
  pthread_mutex_destroy(&producer_finished_lock);
  pthread_cond_destroy(&cond_not_empty);
  pthread_cond_destroy(&cond_not_full);

  return 0;
}