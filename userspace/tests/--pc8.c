/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 666]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"
#include "stdlib.h"


int function_pclast()
{
    return 0;
}  

// this test check if the process is killed because of wrong parameter (it should)
// therefore we create a child to let it die for us
int main()
{
  pid_t pid = fork();
  if (pid == 0) {
    //Test7: Invalid userspace address as thread_id
    pthread_create((void*)0x7ffffffa0000, NULL, (void * (*)(void *))function_pclast, NULL);
    exit(12345);  // this should never be reached
    return -1;
  } 
  else if (pid > 0) 
  {
    int retval_child;
    waitpid(pid, &retval_child, 0);
    if (retval_child != 12345)
    {
      return 0;
    }
    return -1;
  } 
  else
  {
    // Fork failed
    printf("Fork failed\n");
    return -1;
  }
  return -1;
}

