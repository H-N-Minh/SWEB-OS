#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"

#define STACK_SIZE2 (8 * 1024) // 8KB

int gs2()
{
  char stack_data[9000];
  stack_data[0] = 'b';
  stack_data[1] = '\0';
  printf("stack_data[0] = %c\n", stack_data[0]);
  // for (int i = 0; i < STACK_SIZE2; i++)
  // {
  //   stack_data[i] = 'A';
  // }
  // stack_data[STACK_SIZE] = '\0';
  // printf("%c\n", stack_data[STACK_SIZE]);

  return 0;
}

