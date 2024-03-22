#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void printNumber(size_t x)
{
  for (size_t i = 0; i < 1000; i++)
  {
    printf("%zu\n", x);
  }
}


int main()
{
  size_t x = 0;
  size_t y = 111111111;
  pthread_t threads[2];

  multi_pthread_create(threads, NULL, (void* (*)(void*))printNumber, (void*) x, 2);
  pthread_create(&threads[1], NULL, (void* (*)(void*))printNumber, (void*) y);

  void* return_value = 0x0;

  for (size_t j = 0; j < 2; j++) {
    pthread_join(threads[j], &return_value);

    printf("Thread %zu Has Returned Value: %p\n", j, return_value);
  }

  printf("both thread has finished\n");



  return 0;
}