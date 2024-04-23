#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"



int fe2()
{
  pid_t pid = fork();

  if (pid < 0) 
  {
      return -1;
  } 
  else if (pid == 0)
  {
    return 0;
  } 
  else
  {
    const char * path = "usr/pthreadCreateTests.sweb";
    char *argv[] = { (char *)0 };

    int rv = execv(path, argv);
    assert(rv);
    assert(0);
  }
  assert(0);
  return 0;
}