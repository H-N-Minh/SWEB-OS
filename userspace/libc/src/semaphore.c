#include "semaphore.h"



/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_init(sem_t *sem, int pshared, unsigned value)
{
  if(!parameters_are_valid((size_t)sem, 0) || sem->initialized_)
  {
    return -1;    //Error: Sem already initalized or sem address not valid
  }
  if (!pshared)
  {
    return -1;    // Shared between processes, not implemented yet
  }
  
  int rv = pthread_mutex_init(&sem->sem_mutex_, NULL);
  assert(rv == 0);
  rv = pthread_cond_init(&sem->sem_cond_, NULL);
  assert(rv == 0);

  sem->count_ = value;
  sem->initialized_ = 1;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_wait(sem_t *sem)
{
  pthread_mutex_lock(&sem->mutex);
  while (sem->count == 0) {
      pthread_cond_wait(&sem->cond, &sem->mutex);
  }
  sem->count--;
  pthread_mutex_unlock(&sem->mutex);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_trywait(sem_t *sem)
{
  int ret = 0;
  pthread_mutex_lock(&sem->mutex);
  if (sem->count > 0) {
      sem->count--;
      ret = 1;
  }
  pthread_mutex_unlock(&sem->mutex);
  return ret;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_post(sem_t *sem)
{
  pthread_mutex_lock(&sem->mutex);
  sem->count++;
  pthread_cond_signal(&sem->cond);
  pthread_mutex_unlock(&sem->mutex);
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_destroy(sem_t *sem)
{
  int rv = pthread_mutex_destroy(&sem->mutex);
  assert(rv == 0);
  rv = pthread_cond_destroy(&sem->cond);
  assert(rv == 0);
  
  sem->initialized_ = 1;
}