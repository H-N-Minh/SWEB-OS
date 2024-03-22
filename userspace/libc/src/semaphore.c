#include "semaphore.h"


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_wait(sem_t *sem)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_trywait(sem_t *sem)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_init(sem_t *sem, int pshared, unsigned value)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_destroy(sem_t *sem)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int sem_post(sem_t *sem)
{
  return -1;
}


//------------SEM-------------------------

//typedef struct {
//    int counter;
//    pthread_t* sleepers;
//} sem_t;
//
//int sem_init(sem_t *sem, int pshared, unsigned counter)
//{
//    sem->counter = counter;
//    sem->sleepers = NULL;
//    return 0;
//}
//
//int sem_wait(sem_t *sem)
//{
//        if (sem->counter <= 0)
//        {
//             __asm__ volatile("pause");
//        }
//        sem->counter-- (cant use -- bc we need atomically)
//        return 0;
//    }
//}
//
//int sem_post(sem_t *sem)
//{
//    sem->counter++ (cant use ++ bc we need atomically)
//    return 0;
//}
//
//int sem_destroy(sem_t *sem)
//{
//    free(sem->sleepers); maybe we need to allocate sleepers when sem init? but we dont have malloc
//    return 0;
//}
//
//int sem_trywait(sem_t *sem,)
//{
//    if (sem->counter <= 0)
//    {
//        call returns an error (errno set to EAGAIN) instead of blocking
//    }
//    sem->counter-- (cant use -- bc we need atomically)
//    return 0;
//}
//}

