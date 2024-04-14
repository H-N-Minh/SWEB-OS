#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "pthread.h"
#include "types.h"


int flag1 = 0;
void print_function()
{
  while(1)
  {
    printf(". ");
    flag1 = 1;
  }   
}

//Test: Exec with arguments
int exec6()
{
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {

    pthread_t thread_id;
    int rv = pthread_create(&thread_id, NULL, (void*)print_function, NULL);
    assert(rv == 0);

    while(!flag1){}

    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { "6", "Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    execv(path, argv);
    assert(0);    //this should never be reached

    return pid;
  } 
  else //parent
  {
    return pid;
  }
}