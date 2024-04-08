#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

int global_4_1 = 0;

int global_4_2 = 0;

int flag = 0;


pthread_mutex_t mutex_4_1;
pthread_mutex_t mutex_4_2;

void function_1(void* thread_id)
{
  int rv;
  //unlocking a not locked lock should fail
  rv = pthread_mutex_unlock(&mutex_4_1);
  assert(rv != 0);


  rv = pthread_mutex_lock(&mutex_4_1);
  assert(rv == 0);

  //locking and unlocking another lock inside this lock should work
  rv = pthread_mutex_lock(&mutex_4_2);
  assert(rv == 0);
  //locking twice the same lock should return error
  rv = pthread_mutex_lock(&mutex_4_2);
  assert(rv != 0);
  rv = pthread_mutex_unlock(&mutex_4_2);
  assert(rv == 0);

  flag = 1;
  while(flag){}
  rv = pthread_mutex_unlock(&mutex_4_1);
  assert(rv == 0);
}

void function_2(void* thread_id)
{
  int rv;
  while(!flag){}
  //atempt to unlock a lock that is not held by this thread
  rv = pthread_mutex_unlock(&mutex_4_1);
  assert(rv != 0);

  // //trylock a lock currently held by another thread should fail
  // rv = pthread_mutex_trylock(&mutex_4_1);
  // assert(rv != 0);

  flag = 0;

}


int mutex4() {
  pthread_t threads_1;
  pthread_t threads_2;
  int rv;

  rv = pthread_mutex_init(&mutex_4_1, 0);
  assert(rv == 0);
  rv = pthread_mutex_init(&mutex_4_2, 0);
  assert(rv == 0);


  rv = pthread_create(&threads_1, NULL, (void* (*)(void*))function_1, (void*)1);
  assert(rv == 0);
  rv = pthread_create(&threads_2, NULL, (void* (*)(void*))function_2, (void*)2);
  assert(rv == 0);

  rv = pthread_join(threads_1, NULL);
  assert(rv == 0);
  rv = pthread_join(threads_2, NULL);
  assert(rv == 0);
 

  rv = pthread_mutex_destroy(&mutex_4_1);
  assert(rv == 0);
  rv = pthread_mutex_destroy(&mutex_4_2);
  assert(rv == 0);

  printf("mutex4 successful!\n");

  return 0;
}

