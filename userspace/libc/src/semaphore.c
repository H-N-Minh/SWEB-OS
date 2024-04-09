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
  
  int rv = pthread_mutex_init(&sem->sem_mutex_, 0);
  assert(rv == 0);
  rv = pthread_cond_init(&sem->sem_cond_, 0);
  assert(rv == 0);

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

  int rv = pthread_mutex_lock(&sem->sem_mutex_);
  assert(rv == 0);
  while (sem->count_ == 0) {
      rv = pthread_cond_wait(&sem->sem_cond_, &sem->sem_mutex_);
      assert(rv == 0);
  }
  sem->count_--;
  rv = pthread_mutex_unlock(&sem->sem_mutex_);
  assert(rv == 0);
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
  int rv = pthread_mutex_lock(&sem->sem_mutex_);
  assert(rv == 0);
  if (sem->count_ > 0) {
      sem->count_--;
      retval = 0;
  }
  rv = pthread_mutex_unlock(&sem->sem_mutex_);
  assert(rv == 0);
  return retval;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_post(sem_t *sem)
{
  return -1;
  // pthread_mutex_lock(&sem->mutex);
  // sem->count++;
  // pthread_cond_signal(&sem->cond);
  // pthread_mutex_unlock(&sem->mutex);
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_destroy(sem_t *sem)
{
  int rv = pthread_mutex_destroy(&sem->sem_mutex_);
  assert(rv == 0);
  rv = pthread_cond_destroy(&sem->sem_cond_);
  assert(rv == 0);

  sem->initialized_ = 1;
  return 0;
}