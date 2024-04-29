#include "pthread.h"
#include "stdlib.h"
#include "stdio.h"
#include "assert.h"



void e1_1()     //exit in main
{
    printf("e1 successful if no assertion gets raised!\n");
    exit(EXIT_SUCCESS);
    assert(0);
}

int e1()
{
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    e1_1();
    assert(0);    //this should never be reached

    return pid;
  } 
  else //parent
  {
    return 0;
  }
}
