#include "pthread.h"
#include "stdio.h"
#include "assert.h"

int function_pj8()
{
    return 6;
}

//sanity checks
int pj8()
{
  pthread_t thread_id;
  int pthread_create_rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function_pj8, NULL);
  assert(pthread_create_rv == 0);



  //No thread id provided
  void* value_ptr;
  int pthread_join_rv = pthread_join(NULL, &value_ptr);
  assert(pthread_join_rv != 0);

  //Wrong thread id provided
  void* value_ptr;
  int pthread_join_rv = pthread_join((void*)27, &value_ptr);
  assert(pthread_join_rv != 0);

  //value



    return 0;
}