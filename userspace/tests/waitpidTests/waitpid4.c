#include "stdio.h"
#include "stdlib.h"
#include "wait.h"
#include "unistd.h"
#include "assert.h"

int waitpid4()
{
  pid_t pid = 44;
  int status;

  int rv = waitpid(pid, &status, 0);
  

  return 0;
}
