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
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    return -1;
  }
  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  if(mutex->locked_ || mutex->waiting_list_)
  {
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
    return -1;
  }
  else
  {
    mutex->initialized_ = 0;
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
  }
  rv = pthread_spin_destroy(&mutex->mutex_lock_);
  return rv;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)              //TODOs: locking
{
  if(!parameters_are_valid((size_t)mutex, 0) || !parameters_are_valid((size_t)attr, 1) || mutex->initialized_ == MUTEX_INITALIZED)
  {
    return -1;
  }
  int rv = pthread_spin_init(&mutex->mutex_lock_, 0);
  if(rv != 0)
  {
    return -1;
  }
  rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);
  mutex->initialized_ = MUTEX_INITALIZED;
  mutex->locked_ = 0;
  mutex->held_by_ = 0;
  mutex->waiting_list_ = 0;
  rv = pthread_spin_unlock(&mutex->mutex_lock_);
  assert(rv == 0);
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

  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);
  size_t stack_variable;
  size_t* waiting_list_address = (size_t*)((size_t)&stack_variable + 4080 - (size_t)(&stack_variable)%4096);   
  size_t* waiting_flag_address = (size_t*)((size_t)&stack_variable + 4088 - (size_t)(&stack_variable)%4096);   

  if(mutex->held_by_ == waiting_list_address)
  {
    //printf("Thread is already holding lock\n");
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  int counter1 = 0;
  while (mutex->locked_)
  {
    counter1++;
    //*waiting_list_address =(size_t)&mutex->waiting_list_;
    //mutex->waiting_list_ = waiting_list_address;
    size_t* next_element = (size_t*)&mutex->waiting_list_;  
    int added_to_waiting_list = 0;
    while(!added_to_waiting_list)
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
        added_to_waiting_list = 1;
        //printf("2next element %p\n", next_element);
        break;
      }
    }
    pthread_spin_unlock(&mutex->mutex_lock_);
    *waiting_flag_address = 1;
    __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
    //pthread_spin_lock(&mutex->mutex_lock_);
  }
  if(counter1 != 0 && counter1 != 1)
    assert(0 && "yield more than once");
    //printf("counterabc %d\n", counter1);
  mutex->locked_ = 1;
  mutex->held_by_ = waiting_list_address;
  rv= pthread_spin_unlock(&mutex->mutex_lock_);
  assert(rv == 0);
 
  

  
  //assert(mutex->held_by_ == waiting_list_address && "Held by differs from thread currently holding the lock");
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

  mutex->locked_ = 0;
  mutex->held_by_ = 0;
  if(mutex->waiting_list_ != NULL)
  {
    size_t* thread_to_wake_up_ptr = ((size_t*)((size_t)mutex->waiting_list_ + (size_t)8));
    mutex->waiting_list_ = (size_t*)*mutex->waiting_list_;
    *thread_to_wake_up_ptr = 2;
    mutex->mutex_lock_.held_by_ = (size_t*)((size_t)thread_to_wake_up_ptr + (size_t)8);
    //rv = pthread_spin_unlock(&mutex->mutex_lock_);
    //assert(rv == 0);

  }
  else
  {
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
  }
    
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    printf("parameters not valid or lock not initialized\n");
    return -1;
  }

  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);
  size_t stack_variable;
  size_t* waiting_list_address = (size_t*)((size_t)&stack_variable + 4080 - (size_t)(&stack_variable)%4096);   
  size_t* waiting_flag_address = (size_t*)((size_t)&stack_variable + 4088 - (size_t)(&stack_variable)%4096);   

  if(mutex->held_by_ == waiting_list_address)
  {
    printf("Thread is already holding lock\n");
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }

  size_t* next_element = (size_t*)&mutex->waiting_list_;  
  if(!mutex->locked_)
  {
    mutex->locked_ = 1;
    mutex->held_by_ = waiting_list_address;
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
  }
  else
  {
    int rv = pthread_spin_unlock(&mutex->mutex_lock_);
    return -1;
  }
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
  if(!parameters_are_valid((size_t)lock, 0) || lock->initialized_ == SPINLOCK_INITALIZED)
  {
    //Error: Spinlock already initalized or lock address not valid
    return -1;
  }

  lock->locked_ = 0;
  lock->initialized_ = SPINLOCK_INITALIZED;
  lock->held_by_ = 0;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_lock(pthread_spinlock_t *lock)
{
  size_t stack_variable;
  size_t* current_thread_ptr = (size_t*)((size_t)&stack_variable + 4096 - (size_t)(&stack_variable)%4096);   
  
  if(!parameters_are_valid((size_t)lock, 0) || lock->initialized_ != SPINLOCK_INITALIZED || lock->held_by_ == current_thread_ptr)
  {
    //lock not initalized or invalid lock_ptr or lock is allready held by current thread
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
  lock->held_by_ = current_thread_ptr;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_trylock(pthread_spinlock_t *lock)
{
  size_t stack_variable;
  size_t* current_thread_ptr = (size_t*)((size_t)&stack_variable + 4096 - (size_t)(&stack_variable)%4096);
  if(!parameters_are_valid((size_t)lock, 0) || lock->initialized_ != SPINLOCK_INITALIZED || lock->held_by_ == current_thread_ptr)
  {
    //lock not initalized or invalid lock_ptr or lock is allready held by current thread
    return -1;
  }
 
  size_t old_val = 1;
  asm("xchg %0,%1"
  : "=r" (old_val)
  : "m" (lock->locked_), "0" (old_val)
  : "memory");

  if(old_val == 0)
  {
    lock->held_by_ = current_thread_ptr;
    return 0;
  }
  else
  {
    return -1;
  }
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_spin_unlock(pthread_spinlock_t *lock)
{
  size_t stack_variable;
  size_t* current_thread_ptr = (size_t*)((size_t)&stack_variable + 4096 - (size_t)(&stack_variable)%4096);   

  if(!parameters_are_valid((size_t)lock, 0) || lock->initialized_ != SPINLOCK_INITALIZED || !lock->locked_ || current_thread_ptr != lock->held_by_)
  {
    //lock not initalized, not locked or invalid lock_ptr, not held by current thread
    return -1;
  }

  size_t old_val = 0;
  asm("xchg %0,%1"
          : "=r" (old_val)
          : "m" (lock->locked_), "0" (old_val)
          : "memory");
  lock->held_by_ = 0;
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

