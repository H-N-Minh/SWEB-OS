#include "semaphore.h"
#include "assert.h"


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_init(sem_t *sem, int pshared, unsigned value)
{
  return -1;
  if(!parameters_are_valid((size_t)sem, 0) || sem->initialized_)
  {
    return -1;    //Error: Sem already initalized or sem address not valid
  }
  if (!pshared)
  {
    return -1;    // Shared between processes, not implemented yet
  }
  
  int rv = pthread_mutex_init(&sem->count_mutex_, 0);
  assert(rv == 0 && "failed to init mutex in sem_init");
  rv = pthread_cond_init(&sem->count_cond_, 0);
  assert(rv == 0 && "failed to init cond in sem_init");

  sem->count_ = value;
  sem->initialized_ = 1;
  return 0;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_wait(sem_t *sem)
{
  if(!parameters_are_valid((size_t)sem, 0) || !sem->initialized_)
  {
    return -1;    //Error: Sem not initalized or sem address not valid
  }

  int rv = pthread_mutex_lock(&sem->count_mutex_);
  assert(rv == 0 && "failed to lock mutex in sem_wait");

  while (sem->count_ == 0) {
      rv = pthread_cond_wait(&sem->count_cond_, &sem->count_mutex_);
      assert(rv == 0 && "failed to cond_wait in sem_wait");
  }
  sem->count_--;

  rv = pthread_mutex_unlock(&sem->count_mutex_);
  assert(rv == 0 && "failed to unlock mutex in sem_wait");
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_trywait(sem_t *sem)
{
  if(!parameters_are_valid((size_t)sem, 0) || !sem->initialized_)
  {
    return -1;    //Error: Sem not initalized or sem address not valid
  }

  int retval = -1;
  int rv = pthread_mutex_lock(&sem->count_mutex_);
  assert(rv == 0 && "failed to lock mutex in sem_trywait");
  if (sem->count_ > 0) {
      sem->count_--;
      retval = 0;
  }
  rv = pthread_mutex_unlock(&sem->count_mutex_);
  assert(rv == 0 && "failed to unlock mutex in sem_trywait");
  return retval;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_post(sem_t *sem)
{
  if(!parameters_are_valid((size_t)sem, 0) || !sem->initialized_)
  {
    return -1;    //Error: Sem not initalized or sem address not valid
  }

  int rv = pthread_mutex_lock(&sem->count_mutex_);
  assert(rv == 0 && "failed to lock mutex in sem_post");

  sem->count_++;
  rv = pthread_cond_signal(&sem->count_cond_);
  assert(rv == 0 && "failed to send signal in sem_post");

  rv = pthread_mutex_unlock(&sem->count_mutex_);
  assert(rv == 0 && "failed to unlock mutex in sem_post");
  return 0;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_destroy(sem_t *sem)
{
  if(!parameters_are_valid((size_t)sem, 0) || !sem->initialized_)
  {
    return -1;    //Error: Sem not initalized or sem address not valid
  }

  int rv = pthread_mutex_destroy(&sem->count_mutex_);
  assert(rv == 0 && "failed to destroy mutex in sem_destroy");
  rv = pthread_cond_destroy(&sem->count_cond_);
  assert(rv == 0 && "failed to destroy cond in sem_destroy");

  sem->initialized_ = 1;
  return 0;
}