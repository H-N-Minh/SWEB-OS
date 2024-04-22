#include "stdio.h"
#include "pthread.h"
#include "assert.h"


int function_pclast()
{
  return 0;
}  

//Test: Invalid userspace addresss as value_ptr
void pjlast()
{
  pthread_t thread_id;
  int pthread_create_rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function_pclast, NULL);
  assert(pthread_create_rv == 0);

  pthread_join(thread_id, (void*)0x7ffffffa0000);
  assert(0);

}



        
