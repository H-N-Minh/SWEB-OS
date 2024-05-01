#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"

#define STACK_SIZE2 (4 * 4096 - 6*8) // 4 pages - 6*8 bytes of metadata
#define PAGE_SIZE2 4096
#define STACK_AMOUNT2 4

size_t getTopOfStack2(size_t stack_variable)
{
  size_t top_stack = stack_variable - stack_variable%PAGE_SIZE2 + PAGE_SIZE2 - sizeof(size_t);
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  printf("debug: top_stack is: %zu\n", top_stack);
  return top_stack;
}

void* growingFoward2()
{
  size_t p;
  p = (size_t) &p;

  size_t top_stack = getTopOfStack2(p);
  size_t bottom_stack = top_stack - PAGE_SIZE2*STACK_AMOUNT2 + sizeof(size_t) + 1;
  printf("debuging: got here\n");
  int debug_counter = 0;
  while (p >= bottom_stack)
  {
    // debug_counter++;
    // if (debug_counter > 16245)
    // {
    //   printf("debug is at %d, about %zu away\n", debug_counter, p - bottom_stack);
    // }

    p -= 1;
    *(char*) p = 'F';
  }
  printf("debuging: got here 2\n");
  debug_counter = 0;
  while (p < (size_t) &p)
  {
    debug_counter++;
    if (debug_counter > 15465)
    {
      printf("p is at %zu, about %zu away\n", p, top_stack - p);
    }
    if (*(char*) p != 'F')
    {
      return (void*) -1;
    }
    p += 1;
  }
  printf("debuging: got here 3\n");

  if (*(size_t*) p != (size_t) &p)
  {
    return (void*) -1;
  }

  printf("debuging: got here 4\n");
  
  return (void*) 0;
}

void* growingFoward2_2()
{
  printf("debuging: got heree 1\n");
  char stack_data[STACK_SIZE2];
  for (int i = 0; i < STACK_SIZE2; i++)
  {
    stack_data[i] = 'A';
  }
  printf("debuging: got heree 2\n");
  
  for (size_t i = (STACK_SIZE2 - 1); i >= 0; i--)
  {
    if (stack_data[i] != 'A')
    {
      return (void*) -1;
    }
  }
  return (void*) 0;
}



// 2 threads grow from 1st page to last page (1 with array and 1 just manually byte by byte)
// they all should success with return value 0
int gs2()
{
  pthread_t thread1, thread2;
  
  if (pthread_create(&thread1, NULL, growingFoward2, NULL) != 0)
  {
    printf("Failed to create thread 1\n");
    return -1;
  }
  int retval1 = -1;
  if (pthread_join(thread1, (void**) &retval1) != 0)
  {
    printf("Failed to join thread 1\n");
    return -1;
  }
  if (retval1 != 0)
  {
    printf("growingFoward2() failed\n");
    return -1;
  }
  /////////////////////////////////////////////////
  // if (pthread_create(&thread2, NULL, growingFoward2_2, NULL) != 0)
  // {
  //   printf("Failed to create thread 2\n");
  //   return -1;
  // }
  // int retval2 = 0;
  // if (pthread_join(thread2, (void**) &retval2) != 0)
  // {
  //   printf("Failed to join thread 2\n");
  //   return -1;
  // }
  // if (retval2 != 0)
  // {
  //   printf("growingFoward2_2() failed\n");
  //   return -1;
  // }

  return 0;
}
