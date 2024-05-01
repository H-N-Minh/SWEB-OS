#include <stdio.h>
#include <pthread.h>
#include "assert.h"

pthread_t thread_function(void* arg)
{
  // printf("Thread ID: %lu\n", pthread_self());
  return pthread_self();
}

int threadSelfTest()
{
  pthread_t thread = pthread_self();
  // printf("Thread ID of main: %lu\n",thread);
  pthread_t thread1, thread2;

  pthread_create(&thread1, NULL, (void * (*)(void *))thread_function, NULL);
  pthread_create(&thread2, NULL, (void * (*)(void *))thread_function, NULL);

  void* return_value;
  pthread_join(thread1, &return_value);
  assert(return_value == (void*)thread1);
  pthread_join(thread2, &return_value);
  assert(return_value == (void*)thread2);

  return 0;
}
