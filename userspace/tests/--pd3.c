/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "pthread.h"
#include "assert.h"
#include "stdio.h"

//A detached thread should not be joinable

void pd3_function()
{
  while(1)
  {
    sleep(1);
  }
}

void pd3_function_2()
{
  return;
}


int main()
{
  //test for alive function
  pthread_t tid;
  assert(pthread_create(&tid, NULL, (void*)pd3_function, NULL) == 0);
  assert(pthread_detach(tid) == 0);
  assert(pthread_join(tid, NULL) != 0);

  //test for dead function
  pthread_t tid_2;
  assert(pthread_create(&tid_2, NULL, (void*)pd3_function_2, NULL) == 0);
  sleep(1);
  assert(pthread_detach(tid_2) == 0);
  assert(pthread_join(tid_2, NULL) != 0);

  return 0;

}