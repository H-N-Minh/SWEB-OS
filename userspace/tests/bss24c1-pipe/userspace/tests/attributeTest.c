#include <pthread.h>
#include <stdio.h>
#define NUM_THREADS 1

void thread_function(void *arg)
{
  int thread_id = *((int*)arg);
  printf("Hello from thread %d\n", thread_id);
}

int main() {

  pthread_t threads[NUM_THREADS];

  pthread_attr_t attr;
  pthread_t thread;
  size_t stack_size;

  pthread_attr_init(&attr);

  printf("-----------------------BEFORE-------------------\n");
  printf("Stack size set: %zu bytes\n", attr.stack_size);
  printf("Detach state: %d\n", attr.detach_state);
  //printf("Stack address: %p\n", attr.stack_addr);
  printf("Priority: %d\n", attr.priority);

  pthread_attr_setstacksize(&attr, 1024 * 1024); // 1 MB stack size

  printf("-----------------------AFTER-------------------\n");
  printf("Stack size set: %zu bytes\n", attr.stack_size);
  printf("Detach state: %d\n", attr.detach_state);
  //printf("Stack address: %p\n", attr.stack_addr);
  printf("Priority: %d\n", attr.priority);

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_create(&threads[i], &attr, (void * (*)(void *))thread_function, (void *)0);
  }



  return 0;
}
