#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"

#define STACK_SIZE2 (8 * 1024) // 8KB

int gs2()
{
  char stack_data[STACK_SIZE2 + 1];
  for (int i = 0; i < STACK_SIZE2; i++)
  {
    stack_data[i] = 'A';
  }
  // the string must be terminated with a null character
  stack_data[STACK_SIZE2] = '\0';
  assert(stack_data[0] == 'A' && "start of string is not 'A'");
  assert(stack_data[STACK_SIZE2 - 1] == 'A' && "end of string is not 'A'");

  return 0;
}

