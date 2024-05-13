/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 666]
disabled: false
*/

#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"

#define PAGE_SIZE4 4096
#define STACK_AMOUNT4 5   // make sure this alligns with the define in UserSpaceMemoryManager.h


// accessing above stack range, which should crash
void childFunc4_1()
{
    int i = 0;
    // printf("top of this page is %p (%zu)\n", (void*) getTopOfThisPage((size_t) &i), getTopOfThisPage((size_t) &i));
    int *p = &i;
    // printf("gs1: p = %p (%zu) with value %d (should be 0)\n", p, (size_t) p, *p);

    p =(int*) ((size_t)p + PAGE_SIZE4);
    // printf("now accessing new p at %p (%zu) \n", p, (size_t) p);
    *p = 11;
}

// accessing under stack range, which should crash
void childFunc4_2()
{
    int i = 0;
    // printf("top of this page is %p (%zu)\n", (void*) getTopOfThisPage((size_t) &i), getTopOfThisPage((size_t) &i));
    int *p = &i;
    // printf("gs1: p = %p (%zu) with value %d (should be 0)\n", p, (size_t) p, *p);

    p =(int*) ((size_t)p - PAGE_SIZE4 * STACK_AMOUNT4);
    // printf("now accessing new p at %p (%zu) \n", p, (size_t) p);
    *p = 11;
}



// Testing gs4: invalid growing 1: try to access outside stack limit
int main()
{
  pid_t pid = fork();
  if (pid == 0)
  {
    childFunc4_1();
    exit(0);
  }
  else
  {
    int status;
    waitpid(pid, &status, 0);
    if (status == 0)
    {
      printf("Child process didnt crash as it should\n");
      return -1;
    }
    // printf("Child process crashed as expected with code %d\n", status);
  }

  ///////////////////////////////////////////////////////////////
  pid_t pid2 = fork();
  if (pid2 == 0)
  {
    childFunc4_2();
    exit(0);
  }
  else
  {
    int status2;
    waitpid(pid2, &status2, 0);
    if (status2 == 0)
    {
      printf("Child process didnt crash as it should\n");
      return -1;
    }
    // printf("Child process crashed as expected with code %d\n", status);
  }
  
  return 0;
}
