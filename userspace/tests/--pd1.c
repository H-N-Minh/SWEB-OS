/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
#include "assert.h"

int pd1_function()
{
  sleep(1);
  return 0;
}

int main()
{
  //pthread detach should return error if thread cannot be found
  int rv  = pthread_detach(29387);
  assert(rv != 0);


  pthread_t pid;
  rv = pthread_create(&pid, NULL, (void*)pd1_function, NULL);
  assert(rv == 0);
  rv = pthread_detach(pid);
  assert(rv == 0);
  //pthread join after detach should fail
  rv = pthread_join(pid, NULL);
  assert(rv != 0);

  pthread_t pid2;
  rv = pthread_create(&pid2, NULL, (void*)pd1_function, NULL);
  assert(rv == 0);

  rv = pthread_detach(pid2);
  assert(rv == 0);
  //a nonjoinable thread should not be able to get detached
  rv = pthread_detach(pid2);
  assert(rv != 0);

  
  return 0;
}