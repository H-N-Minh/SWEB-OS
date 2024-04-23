#include "pthread.h"
#include "assert.h"
#include "stdio.h"

// //detach pthread on which join is waiting and check if it really isnt joinable

void pd2_function()
{
  while(1)
  {
    sleep(1);
  }
}

void pd2_function_2(pthread_t tid)
{
  assert(pthread_join(tid, NULL) == 0);
}




int pj10()
{
  pthread_t tid;
  assert(pthread_create(&tid, NULL, (void*)pd2_function, NULL) == 0);

  sleep(2);

  pthread_t tid_2;
  assert(pthread_create(&tid_2, NULL, (void*)pd2_function_2, (void*)tid) == 0);

  sleep(2);

  assert(pthread_detach(tid) == 0);

  assert(pthread_cancel(tid) == 0);

  assert(pthread_join(tid, NULL) == -1);

  assert(pthread_join(tid_2, NULL) == 0);

  assert(pthread_join(tid, NULL) == -1);

  return 0;

}




