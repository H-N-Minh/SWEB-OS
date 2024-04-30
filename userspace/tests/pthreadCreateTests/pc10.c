#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"
#include "sched.h"

void* thread_func10_1(void* arg)
{
  // Code for thread 1
  return NULL;
}

void* thread_func10_2(void* arg)
{
  // Code for thread 2
  return NULL;
}

int pc10()
{
  pthread_t thread1, thread2;
  int rv1, rv2;
  
  rv1 = pthread_create(&thread1, NULL, thread_func10_1, NULL);
  assert(rv1 == 0);
  
  rv2 = pthread_create(&thread2, NULL, thread_func10_2, NULL);
  assert(rv2 == 0);
  
  // Wait for both threads to finish
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  
  return 0;
}
