#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "pthread.h"

#define NUM_THREADS 30

//Many exec and forks
int flag11 = 0;

int exec11_1()
{
  while(!flag11){sched_yield();}
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { "11", "1", "abc", (char *)0 };

    execv(path, argv);
    assert(0);
  } 
  else //parent
  {
    return 0;
  }
  return -1;
}

int exec11_2()
{
  while(!flag11){sched_yield();}
  pid_t pid = fork();

  if (pid == -1)
  {
    return -1;
  } 
  else if (pid == 0) //Child
  {
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { "11", "2", "cde", (char *)0 };

    execv(path, argv);
    assert(0);
  } 
  else //parent
  {
    return 0;
  }
  return -1;
}

int exec11()
{
  pthread_t thread_id[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++)
  {
    if(i%2 == 0)
    {
      int rv = pthread_create(&thread_id[i], NULL, (void*)exec11_1, NULL);
      assert(rv == 0);
    }
    else
    {
      int rv = pthread_create(&thread_id[i], NULL, (void*)exec11_2, NULL);
      assert(rv == 0);
    }
    
  }
  flag11 = 1;

  for(int i = 0; i < NUM_THREADS; i++)
  {
    int rv = pthread_join(thread_id[i], NULL);
    assert(rv == 0);
  }
  return 0;
}