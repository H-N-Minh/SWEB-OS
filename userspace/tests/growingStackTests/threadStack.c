#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"

#define STACK_SIZE (7 * 1024) // 8KB

int writeToStack()
{
  char stack_data[STACK_SIZE];

  for (int i = 0; i < STACK_SIZE; i++)
  {
    stack_data[i] = 'A';
  }

  printf("%s\n", stack_data);

  return 0;
}

