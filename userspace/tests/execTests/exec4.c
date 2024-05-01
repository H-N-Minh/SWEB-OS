#include "assert.h"
#include "unistd.h"


//Test: Exec with arguments
int exec4_1()
{
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { "4", "Alle meine Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    int rv = execv(path, argv);

    assert(rv);
    assert(0);    //this should never be reached
    return 0;
}


int exec4()
{
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    exec4_1();
    assert(0);    //this should never be reached

    return pid;
  } 
  else //parent
  {
    return 0;
  }
}