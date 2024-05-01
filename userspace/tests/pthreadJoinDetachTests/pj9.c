#include <stdio.h>
#include <pthread.h>
#include <assert.h>

void* thread_function(void* arg)
{
  return NULL;
}

//Try to join thread with itself and try to join already joined thread

int pj9() {
  pthread_t thread;
  pthread_create(&thread, NULL, thread_function, NULL);

  //printf("Attempting to join the thread with itself...\n");
  int rv1 = pthread_join(pthread_self(), NULL);
  assert(rv1 == -1);

  //printf("Attempting to join the thread that has already been joined...\n");
  pthread_join(thread, NULL);
  int rv = pthread_join(thread, NULL);
  assert(rv == -1);
  printf("Test success...\n");

  return 0;
}
