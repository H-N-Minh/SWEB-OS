#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define STACK_SIZE (7 * 1024) // 8KB

void thread_function(void *arg)
{
  char stack_data[STACK_SIZE];

  for (int i = 0; i < STACK_SIZE; i++)
  {
    stack_data[i] = 'A';
  }

  printf("%s\n", stack_data);
}

int main() {
  pthread_t tid;

  pthread_create(&tid, NULL, (void * (*)(void *))thread_function, NULL);

  pthread_join(tid, NULL);


  return 0;
}
