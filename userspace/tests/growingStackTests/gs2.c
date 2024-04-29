#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"

#define STACK_SIZE2 (4 * 4096 - 6) // 4 pages - 6 bytes of metadata


void* growingFoward()
{
  char stack_data[STACK_SIZE2];
  for (int i = 0; i < STACK_SIZE2; i++)
  {
    stack_data[i] = 'A';
  }

  
  for (size_t i = (STACK_SIZE2 - 1); i >= 0; i--)
  {
    if (stack_data[i] != 'A')
    {
      return (void*) -1;
    }
  }
  return (void*) 0;
}

void* growingFoward2()
{
  int x = 0;
  size_t p = (size_t) &x;

  for (int i = 0; i < STACK_SIZE2; i++)
  {
    *(int*) (p + i) = 'B';
  }
  
  for (int i = 0; i < STACK_SIZE2; i++)
  {
    if (*(int*) (p + STACK_SIZE2 - 1 - i) != 'B')
    {
      return (void*) -1;
    }
  }
  return (void*) 0;
}

void* growingBackward()
{
  char stack_data[STACK_SIZE2];
  for (int i = (STACK_SIZE2 - 1); i >= 0; i--)
  {
    stack_data[i] = 'C';
  }

  
  for (size_t i = 0; i < STACK_SIZE2; i++)
  {
    if (stack_data[i] != 'C')
    {
      return (void*) -1;
    }
  }
  return (void*) 0;
}

void* growingBackward2()
{
  int x = 0;
  size_t p = (size_t) &x;

  for (int i = (STACK_SIZE2 - 1); i >= 0; i--)
  {
    *(int*) (p + i) = 'D';
  }
  
  for (int i = 0; i < STACK_SIZE2; i++)
  {
    if (*(int*) (p + i) != 'D')
    {
      return (void*) -1;
    }
  }
  return (void*) 0;
}


// 4 threads, 2 will grow from 1st page to last page (1 with array and 1 just manually byte by byte)
//  the other 2 will grow from last page to 1st page (1 with array and 1 just manually byte by byte)
// they all should success with return value 0
int gs2()
{
  pthread_t thread1, thread2, thread3, thread4;
  
  if (pthread_create(&thread1, NULL, growingFoward, NULL) != 0)
  {
    printf("Failed to create thread 1\n");
    return -1;
  }
  int retval1 = 0;
  if (pthread_join(thread1, (void**) &retval1) != 0)
  {
    printf("Failed to join thread 1\n");
    return -1;
  }
  if (retval1 != 0)
  {
    printf("growingFoward() failed\n");
    return -1;
  }
  /////////////////////////////////////////////////
  if (pthread_create(&thread2, NULL, growingFoward2, NULL) != 0)
  {
    printf("Failed to create thread 2\n");
    return -1;
  }
  int retval2 = 0;
  if (pthread_join(thread2, (void**) &retval2) != 0)
  {
    printf("Failed to join thread 2\n");
    return -1;
  }
  if (retval2 != 0)
  {
    printf("growingFoward2() failed\n");
    return -1;
  }
  /////////////////////////////////////////////////
  if (pthread_create(&thread3, NULL, growingBackward, NULL) != 0)
  {
    printf("Failed to create thread 3\n");
    return -1;
  }
  int retval3 = 0;
  if (pthread_join(thread3, (void**) &retval3) != 0)
  {
    printf("Failed to join thread 3\n");
    return -1;
  }
  if (retval3 != 0)
  {
    printf("growingBackward() failed\n");
    return -1;
  }
  /////////////////////////////////////////////////
  if (pthread_create(&thread4, NULL, growingBackward2, NULL) != 0)
  {
    printf("Failed to create thread 4\n");
    return -1;
  }
  int retval4 = 0;
  if (pthread_join(thread4, (void**) &retval4) != 0)
  {
    printf("Failed to join thread 4\n");
    return -1;
  }
  if (retval4 != 0)
  {
    printf("growingBackward2() failed\n");
    return -1;
  }


  return 0;
}
