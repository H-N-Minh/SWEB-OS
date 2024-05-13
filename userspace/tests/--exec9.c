/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/


#include "assert.h"
#include "unistd.h"
#include "pthread.h"
#include "sched.h"

#define NUM_THREADS 50

int flag9 = 0;
//50 Threads try pthreadcreate at the same time
int exec9_1()
{
  while(!flag9){sched_yield();}
  const char * path = "usr/exec_testprogram.sweb";
  char *argv[] = { "9", "slkdfjlsd", "lskdfj", (char *)0 };

  execv(path, argv);
  return 0;
}


int main()
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
    pthread_t thread_id[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++)
    {
      int rv = pthread_create(&thread_id[i], NULL, (void*)exec9_1, NULL);
      assert(rv == 0);
    }
    flag9 = 1;

    for(int i = 0; i < NUM_THREADS; i++)
    {
      int rv = pthread_join(thread_id[i], NULL);
      assert(rv == 0);
    }

    return 0;
  }
}