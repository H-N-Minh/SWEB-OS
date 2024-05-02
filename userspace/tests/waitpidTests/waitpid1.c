#include "stdio.h"
#include "stdlib.h"
#include "wait.h"
#include "unistd.h"
#include "assert.h"

//Simple waitpid where parent waits on running child
int waitpid1()
{
  pid_t pid;
  int status;

  // Create a child process
  pid = fork();

  if (pid < 0)
  {
    // printf("Fork faild\n");
    assert(0 && "Fork faild");
  }
  else if (pid == 0) //Child
  {
    sleep(2);
    //printf("Child process running...\n");
    return 5;
  }
  else //Parent
  {
    int rv = waitpid(pid, &status, 0);
    assert(rv == pid);
    assert(status == 0);
    //printf("Parent process waiting for child to terminate...\n");
  }

  return 0;
}

