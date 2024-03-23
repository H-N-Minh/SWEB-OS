#include "pthread.h"

#include <stdio.h>


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg)
{
    return __syscall(sc_pthread_create, (size_t)start_routine, (size_t)arg, (size_t) thread, (size_t) pthread_create_helper, 0x0);
  return __syscall(sc_pthread_create, (size_t)start_routine, (size_t)arg, (size_t) thread, (size_t) pthread_create_helper, 0x0);
}

/**
 * after thread finished its task, this helper will exit that thread correctly
 * TODO: setup the return value for these pthread call correctly
*/

//-------------------------------------------------------
//wrapper function. In pthread create
void pthread_create_helper(void* start_routine, void* arg)
{
    void* retval = ((void* (*)(void*))start_routine)(arg);
    pthread_exit(retval);
}

//-------------------------------------------------------?


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
void pthread_exit(void *value_ptr)
{
    __syscall(sc_pthread_exit, (size_t) value_ptr, 0x0, 0x0, 0x0, 0x0);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cancel(pthread_t thread)
{
    return __syscall(sc_pthread_cancel, thread, 0x0, 0x0, 0x0, 0x0);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_join(pthread_t thread, void **value_ptr)
{
  return __syscall(sc_pthread_join, (size_t) thread, (size_t) value_ptr, 0x0, 0x0, 0x0);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_detach(pthread_t thread)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_destroy(pthread_cond_t *cond)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_signal(pthread_cond_t *cond)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_broadcast(pthread_cond_t *cond)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    lock->initialized_ = 0;
    return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    if(lock->initialized_)
    {
        return -1;
    }
    lock->locked_ = 0;
    lock->initialized_ = 1;
    return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_lock(pthread_spinlock_t *lock)
{
    size_t old_val = 1;
    do {
        asm("xchg %0,%1"
                : "=r" (old_val)
                : "m" (lock->locked_), "0" (old_val)
                : "memory");
    } while (old_val && !__syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0));
    return 0; // Success
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_trylock(pthread_spinlock_t *lock)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    size_t old_val = 0;
    asm("xchg %0,%1"
            : "=r" (old_val)
            : "m" (lock->locked_), "0" (old_val)
            : "memory");
    return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_setcancelstate(int state, int *oldstate)
{
    return __syscall(sc_pthread_setcancelstate, (size_t)state, (size_t)oldstate, 0x0, 0x0, 0x0);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_setcanceltype(int type, int *oldtype)
{
    return __syscall(sc_pthread_setcanceltype, (size_t)type, (size_t)oldtype, 0x0, 0x0, 0x0);
}

int get_thread_count(void) {
    return __syscall(sc_threadcount, 0x0, 0x0, 0x0, 0x0, 0x0);
}

//------------MUTEX-------------------------
//typedef struct {
//    bool locked;
//    pthread_t held_by;
//    pthread_t* sleepers;
//} Mutex;
//
//void mutex_init(Mutex* mutex) {
//      Mutex lock = malloc()
//    mutex->locked = 0;
//    mutex->held_by = 0;
//    mutex->sleepers = NULL;
//}
//
//void mutex_lock(Mutex* mutex)
//{
//    pthread_t self = pthread_self(); <--assume we implemented pthread_self() in userspace(get the tid of the current thread)
//    while ((&mutex->locked, 1))
//    {
//        //mutex is locked, add current thread to the sleepers list
//        //mutex is locked wait until it becomes available
//        syscall(yield)
//    }
//    //mutex acquired, update held_by pointer
//
//    mutex->held_by = self;
//      bool locked = 1;
//}
//
//void mutex_unlock(Mutex* mutex)
// {
//    if (mutex->held_by != pthread_self())
//    {
//        //attempting to unlock mutex not held by current thread
//    }
//    // Release the lock
//    mutex->held_by = 0;
//}

//lock
//do sth here
//unlock