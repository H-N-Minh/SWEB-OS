#include "pthread.h"
#include "stdio.h"
#include "sched.h"
#include "assert.h"

#include "stdio.h"


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg)
{
  return __syscall(sc_pthread_create, (size_t)thread, (size_t)attr, (size_t)start_routine, (size_t)arg, (size_t)pthread_create_wrapper);
}

//wrapper function. In pthread create
void pthread_create_wrapper(void* start_routine, void* arg)
{
  void* retval = ((void* (*)(void*))start_routine)(arg);
  pthread_exit(retval);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
void pthread_exit(void *value_ptr)
{
  __syscall(sc_pthread_exit, (size_t)value_ptr, 0x0, 0x0, 0x0, 0x0);
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
  return __syscall(sc_pthread_join, (size_t)thread, (size_t)value_ptr, 0x0, 0x0, 0x0);
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


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  // if(!parameters_are_valid((size_t)mutex, 0))
  // {
  //   return -1;
  // }
  // if(mutex->initialized_ != MUTEX_INITALIZED)
  // {
  //   return -1;
  // }
  // if(mutex->locked_)
  // {
  //   return -1;
  // }
  // mutex->initialized_ = 0;
  return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)              //TODOs: locking
{
  if(!parameters_are_valid((size_t)mutex, 0) || !parameters_are_valid((size_t)attr, 1))
  {
    return -1;
  }
  if(mutex->initialized_ == MUTEX_INITALIZED)
  {
    return -1;
  }
  int rv = pthread_spin_init(&mutex->mutex_lock_, 0);
  if(rv != 0)
  {
    return -1;
  }
  mutex->initialized_ = MUTEX_INITALIZED;
  mutex->locked_ = 0;
  mutex->held_by_ = NULL;
  mutex->waiting_list_ = NULL;

    
  return 0;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    printf("parameters not valid or lock not initialized\n");
    return -1;
  }

  pthread_spin_lock(&mutex->mutex_lock_);
  size_t stack_variable;
  size_t* waiting_list_address = (size_t*)((size_t)&stack_variable + 4088 - 8 - (size_t)(&stack_variable)%4096);   
  size_t* waiting_flag_address = (size_t*)((size_t)&stack_variable + 4096 - 8 - (size_t)(&stack_variable)%4096);   
  //printf("waiting_list_address %p and waiting_flag_address %p\n", waiting_list_address, waiting_flag_address);
  if(mutex->held_by_ == waiting_list_address)
  {
    printf("Thread is already holding lock\n");
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }

  if(!mutex->locked_)
  {
    mutex->locked_ = 1;
    mutex->held_by_ = waiting_list_address;
    //printf("Waiting_list_address lock is %p\n", waiting_list_address);
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
  }
  else
  {
    //print_waiting_list(mutex->waiting_list_, 0);



    size_t* next_element = (size_t*)&mutex->waiting_list_;  
    while(1)
    {
      if(*next_element != NULL)
      {
        next_element = (size_t*)(*next_element);
        //printf("1next element %p\n", next_element);
      }
      else
      {
        *next_element =  (size_t)waiting_list_address;
        *waiting_list_address = NULL;
        //printf("2next element %p\n", next_element);
        break;
      }
    }
    // printf("mutex->waiting_list_ %p\n", mutex->waiting_list_);
    // printf("next_thread_on_waiting_list %p\n", *next_thread_on_waiting_list);
   // assert(next_thread_on_waiting_list != NULL && "About to dereferencing a nullptr.");
    //*next_thread_on_waiting_list = waiting_list_address;
    // printf("mutex->waiting_list_ %p\n", mutex->waiting_list_);
    // printf("next_thread_on_waiting_list %p\n\n", *next_thread_on_waiting_list);

    //assert(waiting_flag_address != NULL && "About to dereferencing a nullptr.");
    //print_waiting_list(mutex->waiting_list_, 1);
    
    pthread_spin_unlock(&mutex->mutex_lock_); //TODOs is setting waiting list to 1 really that clever since maybe it dont gets unlocked - probably better to have wait flag at another address
   
    //printf("hey");
    //printf("waiting_flag_address %p\n", waiting_flag_address);
    *waiting_flag_address = 1;
    __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
    // assert(0);

    // pthread_spin_lock(&mutex->mutex_lock_);
    // printf("mutex->held_by_ %p\n", mutex->held_by_);
    // printf("waiting_list_address %p\n", waiting_list_address);
    // printf("waiting_flag_address %p\n\n", waiting_flag_address);
    // assert(mutex->held_by_ == waiting_list_address && "Held by differs from thread currently holding the lock");
    // pthread_spin_unlock(&mutex->mutex_lock_);
    //printf("ho");

  }
  
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    printf("parameter not valid or not initalized\n");
    return -1;
  }
  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);
  if(mutex->locked_ == 0)
  {
    printf("lock that you want to unlock is not locked\n");
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  size_t stack_variable;
  size_t* waiting_list_address = (size_t*)((size_t)&stack_variable + 4088 - 8 - (size_t)(&stack_variable)%4096);     

  if(mutex->held_by_ != waiting_list_address)
  {
    printf("Thread does not hold current lock\n");
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }


  if(mutex->waiting_list_ != NULL)
  {
    mutex->held_by_ = mutex->waiting_list_;
      
    assert(mutex->waiting_list_ != NULL && "About to dereferencing a nullptr.");
    mutex->waiting_list_ = (size_t*)*mutex->waiting_list_;
    assert(mutex->held_by_ != NULL && "About to dereferencing a nullptr.");
    *((size_t*)((size_t)mutex->held_by_ + (size_t)8)) = 2;

      // printf("\n(unlock)mutex->held_by_ %p\n", mutex->held_by_);
      // printf("(unlock)waiting_flag_address %p\n\n", ((size_t*)((size_t)mutex->held_by_ + (size_t)8)));
      //printf("waiting_list_address %p\n", waiting_list_address);
  }
  else
  {
    mutex->locked_ = 0;
    mutex->held_by_ = 0;
  }
    
  rv = pthread_spin_unlock(&mutex->mutex_lock_);
  assert(rv == 0);
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  // if(!parameters_are_valid((size_t)mutex, 0))
  // {
  //   return -1;
  // }
  // pthread_spin_lock(&mutex->mutex_lock_);
  // if(mutex->initialized_ != MUTEX_INITALIZED)
  // {
  //   pthread_spin_unlock(&mutex->mutex_lock_);
  //   return -1;
  // }
  // size_t stack_variable;
  // size_t* thread_address = (size_t*)((size_t)&stack_variable + 4088 - (size_t)(&stack_variable)%4096);
  // //trying to lock the same thread twice
  // if(mutex->held_by_ == thread_address)
  // {
  //   pthread_spin_unlock(&mutex->mutex_lock_);
  //   return -1;
  // }
  
  // if(!mutex->locked_)
  // {
  //   mutex->locked_ = 1;
  //   mutex->held_by_ = thread_address;
  //   pthread_spin_unlock(&mutex->mutex_lock_);
  // }
  // else
  // {
  //   pthread_spin_unlock(&mutex->mutex_lock_);
  //   return -1;
  // }
  return 0;
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
  if(!parameters_are_valid((size_t)lock, 0))
  {
    return -1;
  }
  if(lock->initialized_ != SPINLOCK_INITALIZED)
  {
    return -1;
  }
  if(lock->locked_)
  {
    return -1;
  }
  lock->initialized_ = 0;
  return 0;

}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
  if(!parameters_are_valid((size_t)lock, 0))
  {
    return -1;
  }
  if(lock->initialized_ == SPINLOCK_INITALIZED)
  {
      return -1;
  }
  lock->locked_ = 0;
  lock->initialized_ = SPINLOCK_INITALIZED;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_lock(pthread_spinlock_t *lock)
{
  if(!parameters_are_valid((size_t)lock, 0))
  {
    return -1;
  }
  if(lock->initialized_ != SPINLOCK_INITALIZED)
  {
    return -1;
  }
  size_t old_val = 1;
  do 
  {
    asm("xchg %0,%1"
        : "=r" (old_val)
        : "m" (lock->locked_), "0" (old_val)
        : "memory");
  } while (old_val && !__syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0));
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_trylock(pthread_spinlock_t *lock)
{
  if(!parameters_are_valid((size_t)lock, 0))
  {
    return -1;
  }
  if(lock->initialized_ != SPINLOCK_INITALIZED)
  {
    return -1;
  }

  size_t old_val = 1;
  asm("xchg %0,%1"
  : "=r" (old_val)
  : "m" (lock->locked_), "0" (old_val)
  : "memory");
  return old_val;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_unlock(pthread_spinlock_t *lock)
{
  if(!parameters_are_valid((size_t)lock, 0))
  {
    return -1;
  }
  if(lock->initialized_ != SPINLOCK_INITALIZED)
  {
    return -1;
  }
  if(lock->locked_ == 0)
  {
    return -1;
  }
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


void pthread_testcancel(void)
{
  __syscall(sc_pthread_testcancel, 0x0, 0x0, 0x0, 0x0, 0x0);
}


int get_thread_count(void) {
    return __syscall(sc_threadcount, 0x0, 0x0, 0x0, 0x0, 0x0);
}

int parameters_are_valid(size_t ptr, int allowed_to_be_null)
{
    if(!allowed_to_be_null && ptr == 0)
    {
      return 0;
    }
    if(ptr >= USER_BREAK)
    {
      return 0;
    }
    return 1;
}

void print_waiting_list(size_t* waiting_list, int before)
{
  size_t* next_element = waiting_list;         //(size_t*)&
  if(before == 0)
  {
    printf( "-----WAITING_LIST_BEFORE------\n");
  }
  else
  {
    printf("-----WAITING_LIST_AFTER------\n");
  }
  
  while(next_element != NULL)
  {
    printf("%p - ", next_element);
    next_element = (size_t*)(*next_element);
  }
  printf("%p\n", next_element);
  printf("-------------------------\n");

  sleep(10);
  //printf("Waiting list was %p\n\n", waiting_list);
}

