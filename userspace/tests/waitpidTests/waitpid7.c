#include "stdio.h"
#include "stdlib.h"
#include "wait.h"
#include "unistd.h"
#include "assert.h"
#include "pthread.h"

#define NUM_THREADS 20


int function7()
{  
  pid_t pid;
  int status;

  pid = fork();
  assert(pid >= 0);

  if (pid == 0) //Child
  {
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { "4", "Alle meine Entchen schwimmen auf", "dem See", "schwimmen auf dem See.", (char *)0 };

    execv(path, argv);
    assert(0);

  }
  else //Parent
  {
    int rv = waitpid(pid, &status, 0);
    assert(rv == pid);
    assert(status == 0);
  }  
  return 7;
}

int function7_2()
{  
  pid_t pid;
  int status;

  pid = fork();
  assert(pid >= 0);

  if (pid == 0) //Child
  {
    const char * path = "usr/exec_testprogram.sweb";
    char *argv[] = { 
    "3","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","2",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1","1",
    "1","1","1","1","1","1","1","1","1","1",
    "1","1", (char *)0 };

    execv(path, argv);
    assert(0);

  }
  else //Parent
  {
    int rv = waitpid(pid, &status, 0);
    assert(rv == pid);
    assert(status == 0);
  }  
  return 8;
}

//Waitpid with pthread_create
int waitpid7()
{

  pthread_t thread_id[NUM_THREADS];
  pthread_t thread_id_2[NUM_THREADS];

  for(int i = 0; i < NUM_THREADS; i++)
  {
    int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function7, NULL);
    assert(rv == 0);
    rv = pthread_create(&thread_id_2[i], NULL, (void * (*)(void *))function7_2, NULL);
    assert(rv == 0);
  }

  for(int i = 0; i < NUM_THREADS; i++)
  {
    assert(thread_id[i] != 0);
    void* retval;
    int rv = pthread_join(thread_id[i], &retval);
    assert(rv == 0);
    assert(retval == (void*)7);

    assert(thread_id_2[i] != 0);
    void* retval1;
    rv = pthread_join(thread_id_2[i], &retval1);
    assert(retval1 == (void*)8);
    assert(rv == 0);
  }
    
  return 0;
}