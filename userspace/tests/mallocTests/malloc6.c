#include "assert.h"
#include "stdlib.h"
#include "wait.h"

//Test: Malloc exits savely if its detect overflow when trying to allocate a new element
int malloc6()
{
  pid_t pid = fork();
  if(pid == 0)
  {
    int* ptr_1 = malloc(5 * sizeof(int));
    int* ptr_2 = malloc(5 * sizeof(int));

    assert(ptr_1 < ptr_2 && "To be able to overflow ptr2 it needs to be bigger than ptr_1");

    for(int i = 0; i < 6; i++)
    {
      ptr_1[i] = 234;
    }
    malloc(5 * sizeof(int));

    assert(0 && "This should never be reached.\n");
  }
  else if(pid > 0)
  {
    int rv = waitpid(pid, NULL, NULL);
    assert(rv == pid);
  }
  else
  {
    assert(0);
  }



  return 0;
}