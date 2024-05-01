#include "stdio.h"
#include "stdlib.h"
#include "wait.h"
#include "unistd.h"
#include "assert.h"

int waitpid4()
{
  pid_t pid = 44;
  int status;
  //Waiting for nonexisting process
  int rv = waitpid(pid, &status, 0);
  assert(rv == -1);

  //Waiting for pid smaller equal 0 (not supported by our implementation)
  rv = waitpid(0, &status, 0);
  assert(rv == -1);

  //Waitpid with no status provided
  pid_t pid2 = fork();
  assert(pid2 >= 0 && "Fork faild");
  if (pid2 == 0)
  {
    return 5;
  }
  else
  {
    int rv = waitpid(pid2, NULL, 0);
    assert(rv == pid2);
  }

  //Waitpid with not userspace address as statusptr
  pid_t pid3 = fork();
  assert(pid3 >= 0 && "Fork faild");
  if (pid3 == 0)
  {
    return 5;
  }
  else
  {
    int rv = waitpid(pid3, (void*)0x0000800000000000ULL, 0);
    assert(rv == -1);
  }

  //Waitpid with nonexisting option (options also not supported)
  pid_t pid4 = fork();
  assert(pid4 >= 0 && "Fork faild");
  if (pid4 == 0)
  {
    return 5;
  }
  else
  {
    int rv = waitpid(pid4, &status, 234);
    assert(rv == -1);
  }
  return 0;
}

