#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


#define PAGESIZE 4096
#define PAGES_IN_ARRAY 1200
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE) / 8
  
size_t big_array1[ELEMENTS_IN_ARRAY]; 



//Trigger out of memory
int swappingFork1()
{
  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    big_array1[i * (PAGESIZE / 8)] = (size_t)i;
  }

  for(int i = 0; i < PAGES_IN_ARRAY; i++)
  {
    assert(big_array1[i * (PAGESIZE / 8)] == i);
  }

  pid_t pid = fork();

  if(pid == 0)
  {
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      assert(big_array1[i * (PAGESIZE / 8)] == i);
    }
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      big_array1[i * (PAGESIZE / 8)] = (size_t)i*2;
    }
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      assert(big_array1[i * (PAGESIZE / 8)] == i*2);
    }
    exit(0);
  }
  else if(pid > 0)
  {
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      assert(big_array1[i * (PAGESIZE / 8)] == i);
    }
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      big_array1[i * (PAGESIZE / 8)] = (size_t)i*3;
    }
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
      assert(big_array1[i * (PAGESIZE / 8)] == i*3);
    }

    int status;
    waitpid(pid, &status, 0);
    if (status != 0)
    {
      assert(0 && "Child exits with wrong value.\n");
    }
  }
  else
  {
    assert(0);
  }

  return 0;
}