#include "assert.h"
#include "unistd.h"
#include "pthread.h"


//Test: Exec without arguments
int exec7_1()
{
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { "7", (char *)0 };

    execv(path, argv);
    assert(0);

    return 0;
}


int exec7()
{
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    return 0;
  } 
  else //parent
  {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, (void*)exec7_1, NULL);
    pthread_join(thread_id, NULL);
    assert(0);
    return 0;
  }
}