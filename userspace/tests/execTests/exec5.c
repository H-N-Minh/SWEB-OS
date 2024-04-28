#include "assert.h"
#include "unistd.h"


//Test: Exec without arguments
int exec5_1()
{
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { (char *)0 };

    execv(path, argv);
    assert(0);

    return 0;
}


int exec5()
{
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    exec5_1();
    assert(0);    //this should never be reached

    return pid;
  } 
  else //parent
  {
    return 0;
  }
}