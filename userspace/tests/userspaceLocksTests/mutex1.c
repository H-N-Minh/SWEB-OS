#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex_lock;
int shared_variable = 0;

void* increment(void* arg)
{
  pthread_mutex_lock(&mutex_lock);
  shared_variable++;
  pthread_mutex_unlock(&mutex_lock);
  
  return NULL;
}

int mutex1()
{
  pthread_t thread1, thread2;
  
  pthread_mutex_init(&mutex_lock, NULL);
  
  pthread_create(&thread1, NULL, increment, NULL);
  pthread_create(&thread2, NULL, increment, NULL);
  
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  
  pthread_mutex_destroy(&mutex_lock);
  
  printf("Shared variable value: %d\n", shared_variable);
  
  return 0;
}