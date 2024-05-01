#include <stdio.h>
#include <pthread.h>

void thread_function(void* arg)
{
  printf("Thread ID: %lu\n", pthread_self());
}

int main()
{
  pthread_t thread = pthread_self();
  printf("Thread ID of main: %lu\n",thread);
  pthread_t thread1, thread2;

  pthread_create(&thread1, NULL, (void * (*)(void *))thread_function, NULL);
  pthread_create(&thread2, NULL, (void * (*)(void *))thread_function, NULL);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  return 0;
}
