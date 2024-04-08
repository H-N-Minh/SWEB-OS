#include "pthread.h"
#include "stdio.h"
#include "sched.h"
#include "assert.h"

#include "stdio.h"

#define __PAGE_SIZE__ 4096


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg)
{
  int retval = __syscall(sc_pthread_create, (size_t)thread, (size_t)attr, (size_t)start_routine, (size_t)arg, (size_t)pthread_create_wrapper);
  if (!retval)
  {
    // if top_stack is 0, it means this is the first stack of the parent thread, so it should point to itself
    size_t* top_stack = getTopOfThisStack();
    if(*top_stack == 0)
    {
      *top_stack = (size_t)top_stack;
      // these should already be 0, but just to be sure
      top_stack -= sizeof(size_t);  *top_stack = 0;
      top_stack -= sizeof(size_t);  *top_stack = 0;
    }
  }
  return retval;
}

/**wrapper function. In pthread create
// top_stack points to the top of the 1st stack of the child thread
// Since its the first stack of new thread, it should points to itself */
void pthread_create_wrapper(void* start_routine, void* arg, void* top_stack)
{
  assert(top_stack && "top_stack of Child Thread is NULL");
  *(size_t*) top_stack = (size_t) top_stack;
  // these should already be 0, but just to be sure:
  top_stack -= sizeof(size_t);  *(size_t*) top_stack = 0;   // linked list for waiting threads
  top_stack -= sizeof(size_t);  *(size_t*) top_stack = 0;   // boolean for request_to_sleep
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
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ == SPINLOCK_INITALIZED)
  {
    //Error: Spinlock already initalized or lock address not valid
    return -1;
  }
  if(attr != NULL)  //pshared not implemented
  {
    return -1;
  }

  mutex->locked_ = 0;
  mutex->initialized_ = SPINLOCK_INITALIZED;
  mutex->held_by_ = 0;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)mutex, 0))
  {
    return -1;
  }
  if(mutex->initialized_ != SPINLOCK_INITALIZED)
  {
    return -1;
  }
  if(mutex->locked_)
  {
    return -1;
  }
  mutex->initialized_ = 0;
  return 0;

}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  size_t stack_variable;
  size_t* current_thread_ptr = (size_t*)((size_t)&stack_variable + 4096 - (size_t)(&stack_variable)%4096);   
  
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != SPINLOCK_INITALIZED || mutex->held_by_ == current_thread_ptr)
  {
    //lock not initalized or invalid lock_ptr or lock is allready held by current thread
    return -1;
  }

  size_t old_val = 1;
  do 
  {
    asm("xchg %0,%1"
        : "=r" (old_val)
        : "m" (mutex->locked_), "0" (old_val)
        : "memory");
  } while (old_val && !__syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0));
  mutex->held_by_ = current_thread_ptr;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  size_t stack_variable;
  size_t* current_thread_ptr = (size_t*)((size_t)&stack_variable + 4096 - (size_t)(&stack_variable)%4096);   

  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != SPINLOCK_INITALIZED || !mutex->locked_ || current_thread_ptr != mutex->held_by_)
  {
    //lock not initalized, not locked or invalid lock_ptr, not held by current thread
    return -1;
  }

  size_t old_val = 0;
  asm("xchg %0,%1"
          : "=r" (old_val)
          : "m" (mutex->locked_), "0" (old_val)
          : "memory");
  mutex->held_by_ = 0;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
  if(!parameters_are_valid((size_t)cond, 0) || cond->initialized_)
  {
    return -1;    //Error: Cond already initalized or cond address not valid
  }
  if(attr != NULL)  //attr not implemented
  {
    return -1;
  }

  cond->initialized_ = 1;
  cond->waiting_list_ = 0; 
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_destroy(pthread_cond_t *cond)
{
  if(!parameters_are_valid((size_t)cond, 0) || !cond->initialized_)
  {
    return -1;    //Error: Cond not initalized or cond address not valid
  }
  cond->initialized_ = 0;
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_signal(pthread_cond_t *cond)
{
  if(!parameters_are_valid((size_t)cond, 0) || !cond->initialized_)
  {
    return -1;    //Cond is Null, cond not initialized
  }
  
  if (!cond->waiting_list_)   // if theres at least 1 thread in the waiting list
  { 
    // remove the first thread from the waiting list
    size_t* thread_to_wakeup = (size_t*) cond->waiting_list_;
    cond->waiting_list_ = *thread_to_wakeup;
    // signal the thread to wake up
    size_t* request_to_sleep = thread_to_wakeup - sizeof(size_t);
    assert(*request_to_sleep && "waking a thread that is not sleeping");
    *request_to_sleep = 0;
  }
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_broadcast(pthread_cond_t *cond)
{
  if(!parameters_are_valid((size_t)cond, 0) || !cond->initialized_)
  {
    return -1;    //Cond is Null, cond not initialized
  }
  
  while (!cond->waiting_list_)   // if theres at least 1 thread in the waiting list
  { 
    // remove the first thread from the waiting list
    size_t* thread_to_wakeup = (size_t*) cond->waiting_list_;
    cond->waiting_list_ = *thread_to_wakeup;
    // signal the thread to wake up
    size_t* request_to_sleep = thread_to_wakeup - sizeof(size_t);
    assert(*request_to_sleep && "waking a thread that is not sleeping");
    *request_to_sleep = 0;
  }
  return 0;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)cond, 0) || !cond->initialized_ || !mutex)
  {
    return -1;    //Cond is Null, cond not initialized or mutex is null
  }
  // get the reserved space of current thread
  size_t* new_waiter_stack            = getTopOfFirstStack();
  size_t* new_waiter_list_address     = (size_t*) ((size_t) new_waiter_stack -   sizeof(size_t));
  size_t* new_waiter_request_to_sleep = (size_t*) ((size_t) new_waiter_stack - 2*sizeof(size_t));
  
  // adding curent thread to the waiting list
  size_t last_waiter = getLastCondWaiter(cond);
  *(size_t*)last_waiter = (size_t) new_waiter_list_address;
  
  // current thread signal that it wants to sleep
  // TODO: theres a potential race condition here (lost wake call) (thread is signaled to wake before it can request to sleep)
  assert(!*new_waiter_request_to_sleep && "threads request_to_sleep_ is already true, this should not happen");
  pthread_mutex_unlock(mutex);       
  *new_waiter_request_to_sleep = 1;      // This tells the scheduler that this thread is waiting and can be skipped
  __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
  pthread_mutex_lock(mutex);

  return 0;
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
  if(pshared != NULL)  //pshared not implemented
  {
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

// COND also uses this, dont delete
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


size_t getLastCondWaiter(pthread_cond_t* cond)
{
  size_t* last_waiter = (size_t*)&(cond->waiting_list_);
  while(*last_waiter)
  {
    last_waiter = (size_t*)*last_waiter;
  }
  return (size_t)last_waiter;
}

size_t* getTopOfThisStack() {
  size_t stack_variable;
  size_t* top_stack = (size_t*)((size_t)&stack_variable - (size_t)(&stack_variable)%__PAGE_SIZE__ + __PAGE_SIZE__ - sizeof(size_t)); 
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

size_t* getTopOfFirstStack() {
  size_t* top_current_stack = getTopOfThisStack();
  return (size_t*)*top_current_stack;
}
