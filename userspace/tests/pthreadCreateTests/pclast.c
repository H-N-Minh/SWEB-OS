#include "stdio.h"
#include "pthread.h"
#include "assert.h"


int function_pclast()
{
    return 0;
}  
        
void pclast()
{
  //Test7: Invalid userspace address as thread_id
  pthread_create((void*)0x7ffffffa0000, NULL, (void * (*)(void *))function_pclast, NULL);
  assert(0);
}

