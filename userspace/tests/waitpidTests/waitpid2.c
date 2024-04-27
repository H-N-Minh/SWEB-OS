#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "wait.h"
#include "assert.h"

//Create two childs in the parents and waitpid for both
int waitpid2()
{
  pid_t pid;
  int status;

  pid = fork();

  if (pid < 0)
  {
    // printf("Fork failed\n");
    return 1;
  }
  else if (pid == 0) //Child 1
  {
    // printf("Child process 1 running...\n");
    return 5;
  }
  else //Parent 1
  {
    pid_t pid2 = fork();

    if (pid2 < 0)
    {
      // printf("Fork failed\n");
      return 1;
    }
    else if (pid2 == 0) //Child 2
    {
      // printf("Child process 2 running...\n");
      return 5;
    }
    else //Parent 2
    {
      int rv = waitpid(pid2, &status, 0);
      assert(rv == pid2);
      assert(status == 0);
      // printf("Parent process waiting for child 2 to terminate...\n");

      rv = waitpid(pid, &status, 0);
      assert(rv == pid);
      assert(status == 0);
      // printf("Parent process waiting for child 1 to terminate...\n");
    }
  }

  return 0;
}