#include "semaphore.h"


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_wait(sem_t *sem)
{
  if (!sem->initialized_)
  {
    return -1;
  }

  pthread_spin_lock(&sem->lock);

  while (sem->value == 0)
  {
    pthread_spin_unlock(&sem->lock); //release the lock before yielding
    __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
    pthread_spin_lock(&sem->lock); //reacquire the lock after yielding
  }

  sem->value--;
  pthread_spin_unlock(&sem->lock);

  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_trywait(sem_t *sem)
{
  if (!sem->initialized_)
  {
    return -1;
  }

  pthread_spin_lock(&sem->lock);

  if (sem->value > 0)
  {
    sem->value--;
    pthread_spin_unlock(&sem->lock);
    return 0;
  }

  pthread_spin_unlock(&sem->lock);
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_init(sem_t *sem, int pshared, unsigned value)
{
  if(!sem->initialized_ )
  {
    return -1;//fail
  }
  sem->value = value;
  pthread_spin_init(&sem->lock, 0);
  return 0; //success
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_destroy(sem_t *sem)
{
  if(!sem->initialized_ )
  {
    return -1;
  }
  pthread_spin_destroy(&sem->lock);
  sem->initialized_ = 0;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_post(sem_t *sem)
{
  if(!sem->initialized_ )
  {
    return -1;
  }
  pthread_spin_lock(&sem->lock); //lock for atomicity
  sem->value++;
  pthread_spin_unlock(&sem->lock);

  return 0;
}


