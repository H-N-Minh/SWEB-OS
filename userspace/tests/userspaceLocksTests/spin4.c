#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

pthread_spinlock_t spin_4_1;
pthread_spinlock_t spin_4_2;

int spin_flag = 0;

void spin_function_1(void* thread_id)
{
  int rv;
  //unlocking a not locked lock should fail
  rv = pthread_spin_unlock(&spin_4_1);
  assert(rv != 0);


  rv = pthread_spin_lock(&spin_4_1);
  assert(rv == 0);

  //locking and unlocking another lock inside this lock should work
  rv = pthread_spin_lock(&spin_4_2);
  assert(rv == 0);
  //locking twice the same lock should return error
  rv = pthread_spin_lock(&spin_4_2);
  assert(rv != 0);
  rv = pthread_spin_unlock(&spin_4_2);
  assert(rv == 0);

  spin_flag = 1;
  while(spin_flag){}
  rv = pthread_spin_unlock(&spin_4_1);
  assert(rv == 0);
}

void spin_function_2(void* thread_id)
{
  int rv;
  while(!spin_flag){}
  //atempt to unlock a lock that is not held by this thread
  rv = pthread_spin_unlock(&spin_4_1);
  assert(rv != 0);

  //trylock a lock currently held by another thread should fail
  rv = pthread_spin_trylock(&spin_4_1);
  assert(rv != 0);

  spin_flag = 0;

}


int spin4() {
  pthread_t threads_1;
  pthread_t threads_2;
  int rv;

  rv = pthread_spin_init(&spin_4_1, 0);
  assert(rv == 0);
  rv = pthread_spin_init(&spin_4_2, 0);
  assert(rv == 0);


  rv = pthread_create(&threads_1, NULL, (void* (*)(void*))spin_function_1, (void*)1);
  assert(rv == 0);
  rv = pthread_create(&threads_2, NULL, (void* (*)(void*))spin_function_2, (void*)2);
  assert(rv == 0);

  rv = pthread_join(threads_1, NULL);
  assert(rv == 0);
  rv = pthread_join(threads_2, NULL);
  assert(rv == 0);
 

  rv = pthread_spin_destroy(&spin_4_1);
  assert(rv == 0);
  rv = pthread_spin_destroy(&spin_4_2);
  assert(rv == 0);

  printf("spin4 successful!\n");

  return 0;
}