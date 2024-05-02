#include "pthread.h"
#include "stdio.h"
#include "sched.h"
#include "assert.h"

#include "stdio.h"

#define __PAGE_SIZE__ 4096



size_t pthread_self()
{
  return __syscall(sc_pthread_self, 0x0, 0x0, 0x0, 0x0, 0x0);
}


int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
   return __syscall(sc_pthread_create, (size_t)thread, (size_t)attr, (size_t)start_routine, (size_t)arg, (size_t)pthread_create_wrapper);
}


/**wrapper function. In pthread create
// top_stack points to the top of the 1st stack of the child thread
// Since its the first stack of new thread, it should points to itself */
void pthread_create_wrapper(void* start_routine, void* arg)
{
  // Start the thread actual function
  // DEBUGMINH TODO: delete this
  // printf("pthread_create_wrapper started, start_routine %p, arg %p \n", start_routine, arg);
  void* retval = ((void* (*)(void*))start_routine)(arg);
  // printf("pthread_create_wrapper finished\n");
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
  return __syscall(sc_pthread_detach, thread, 0x0, 0x0, 0x0, 0x0);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    //printf("parameters not valid or lock not initialized\n");
    return -1;
  }
  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);

  size_t top_stack = getTopOfFirstStack();
  assert(top_stack && "top_stack cant be found");
  size_t* mutex_flag = (size_t*) (top_stack - sizeof(size_t));
  size_t* mutex_waiter_list = (size_t*) (top_stack - sizeof(size_t)*2);

  // error handling making sure thread is not holding the lock already
  if(mutex->held_by_ == mutex_waiter_list)
  {
    //printf("Thread is already holding lock\n");
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  // int counter1 = 0;
  while (mutex->locked_)
  {
    // counter1++;

    size_t* next_element = (size_t*)&mutex->waiting_list_;  
    // adding the current thread to the waiting list
    int added_to_waiting_list = 0;
    while(!added_to_waiting_list)
    {
      if(*next_element != NULL)
      {
        next_element = (size_t*)(*next_element);
      }
      else
      {
        *next_element =  (size_t)mutex_waiter_list;
        *mutex_waiter_list = NULL;
        added_to_waiting_list = 1;
        break;
      }
    }
    mutex->mutex_lock_.held_by_ = 0;
    asm("xchg %0,%1"
          : "=r" (*mutex_flag)
          : "m" (mutex->mutex_lock_.locked_), "0" (*mutex_flag)
          : "memory");
    __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
  }

  mutex->locked_ = 1;
  mutex->held_by_ = mutex_waiter_list;
  rv= pthread_spin_unlock(&mutex->mutex_lock_);
  assert(rv == 0);
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
    //printf("parameters not valid or lock not initialized\n");
    return -1;
  }
  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);
  size_t top_stack = getTopOfFirstStack();
  assert(top_stack && "top_stack cant be found");
  size_t* mutex_waiter_list = (size_t*)(top_stack - sizeof(size_t)*2);
  if(mutex->held_by_ == mutex_waiter_list)
  {
    //printf("Thread is already holding lock\n");
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  if(!mutex->locked_)
  {
    mutex->locked_ = 1;
    mutex->held_by_ = mutex_waiter_list;
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
  }
  else
  {
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
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
    //printf("parameter not valid or not initalized\n");
    return -1;
  }
  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);
  if(mutex->locked_ == 0)
  {
    //printf("lock that you want to unlock is not locked\n");
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  size_t top_stack = getTopOfFirstStack();
  assert(top_stack && "top_stack cant be found");
  size_t* mutex_waiter_list = (size_t*)(top_stack - sizeof(size_t)*2);
  if(mutex->held_by_ != mutex_waiter_list)
  {
    // printf("Thread does not hold current lock\n");
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  mutex->locked_ = 0;
  mutex->held_by_ = 0;
  if(mutex->waiting_list_ != NULL)
  {
    size_t* thread_to_wake_up_ptr = ((size_t*)((size_t)mutex->waiting_list_ + (size_t)8));
    mutex->waiting_list_ = (size_t*)*mutex->waiting_list_;
    *thread_to_wake_up_ptr = 0;
    mutex->mutex_lock_.held_by_ = (size_t*)((size_t)thread_to_wake_up_ptr + (size_t)8);
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
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    return -1;
  }
  int rv = pthread_spin_lock(&mutex->mutex_lock_);
  assert(rv == 0);
  if(mutex->locked_ || mutex->waiting_list_)
  {
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  else
  {
    mutex->initialized_ = 0;
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
  }
  rv = pthread_spin_destroy(&mutex->mutex_lock_);
  assert(rv == 0);
  return rv;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
  if(!parameters_are_valid((size_t)mutex, 0) || !parameters_are_valid((size_t)attr, 1))
  {
    return -1;
  }
  int rv = pthread_spin_init(&mutex->mutex_lock_, 0);
  if(rv != 0)
  {
    return -1;
  }
  rv = pthread_spin_lock(&mutex->mutex_lock_);
  if (mutex->initialized_ == MUTEX_INITALIZED)    // last error handling to make sure mutex cant be init twice
  {
    rv = pthread_spin_unlock(&mutex->mutex_lock_);
    assert(rv == 0);
    return -1;
  }
  
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

  if (cond->waiting_list_)   // if theres at least 1 thread in the waiting list
  {
    // remove the first thread from the waiting list and wake it up
    size_t thread_to_wakeup = cond->waiting_list_;
    cond->waiting_list_ = *(size_t*) thread_to_wakeup;
    *(size_t*) thread_to_wakeup = 0;
    size_t* request_to_sleep = (size_t*) (thread_to_wakeup + sizeof(size_t));
    wakeUpThread(request_to_sleep);
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
  while (cond->waiting_list_)   // if theres at least 1 thread in the waiting list
  {
    // remove the first thread from the waiting list and wake it up
    size_t thread_to_wakeup = cond->waiting_list_;
    cond->waiting_list_ = *(size_t*) thread_to_wakeup;
    size_t* request_to_sleep = (size_t*) (thread_to_wakeup + sizeof(size_t));
    wakeUpThread(request_to_sleep);
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
  size_t top_stack                   = getTopOfFirstStack();
  assert(top_stack && "top_stack cant be found");
  size_t cond_flag                   = top_stack - 3*sizeof(size_t);   // this 2 and 3 depends on the order setup in UserThread ctor
  size_t cond_waiter_list            = top_stack - 4*sizeof(size_t);
  assert(top_stack && "top_stack cant be found");

  // adding curent thread to the waiting list
  addWaiterToList(&cond->waiting_list_, cond_waiter_list);
  
  // current thread signal that it wants to sleep
  assert(!*(size_t*) cond_flag && "threads cond_flag_ is already true, this should not happen");
  int rv = pthread_mutex_unlock(mutex);
  assert(rv == 0);
  *(size_t*) cond_flag = 1;      // This tells the scheduler that this thread is waiting and can be skipped
  __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);

  // after waking up, re-acquire the lock
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
  size_t* current_thread_ptr = (size_t*) getTopOfFirstStack();
  assert(current_thread_ptr && "top_stack cant be found");

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
  size_t* current_thread_ptr = (size_t*) getTopOfFirstStack();
  assert(current_thread_ptr && "top_stack cant be found");
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
  size_t* current_thread_ptr = (size_t*) getTopOfFirstStack();
  assert(current_thread_ptr && "top_stack cant be found");
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
  if(ptr >= __USER_BREAK__)
  {
    return 0;
  }
  return 1;
}

void addWaiterToList(size_t* waiting_list_adr, size_t new_waiter)
{
  while(*waiting_list_adr)
  {
    waiting_list_adr = (size_t*)*waiting_list_adr;
  }
  *waiting_list_adr = new_waiter;
  *(size_t*) new_waiter = 0;
}

size_t getTopOfThisStack()
{
  size_t stack_variable;
  size_t top_stack = (size_t)&stack_variable - (size_t)(&stack_variable)%__PAGE_SIZE__ + __PAGE_SIZE__ - sizeof(size_t);
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

size_t getTopOfFirstStack()
{
  size_t top_current_stack = getTopOfThisStack();

  for (size_t i = 0; i < __MAX_STACK_AMOUNT__; i++)
  {
    if (top_current_stack && *(size_t*) top_current_stack == __GUARD_MARKER__)
    {
      return top_current_stack;
    }
    top_current_stack += __PAGE_SIZE__;
  }
  return 0;
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

void wakeUpThread(size_t* request_to_sleep)
{
  size_t old_val = 0;
  do
  {
    asm("xchg %0,%1"
        : "=r" (old_val)
        : "m" (*request_to_sleep), "0" (old_val)
        : "memory");
  } while (!old_val && !__syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0));

  *request_to_sleep = 0;
}

int pthread_attr_init(pthread_attr_t *attr)
{
  if(!parameters_are_valid((size_t)attr, 0))
  {
    return -1;
  }
  if (attr == NULL)
  {
    return -1;
  }
  if (attr->initialized == 1)
  {
    return -1;
  }

  attr->detach_state = PTHREAD_CREATE_JOINABLE; // Default detach state
  attr->stack_addr = NULL; // Default stack address
  attr->stack_size = __DEFAULT_STACK_SIZE__;
  attr->priority = 0;      // Default priority

  attr->initialized = 1;
  return 0; // Success
}

int pthread_attr_destroy(pthread_attr_t *attr) {
  if(!parameters_are_valid((size_t)attr, 0))
  {
    return -1;
  }
  if (attr == NULL || attr->initialized == 0)
  {
    return -1;
  }
  attr->initialized = 0;
  return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
  if(!parameters_are_valid((size_t)attr, 0) || attr->initialized != 1)
  {
    return -1;
  }
  if (attr == NULL)
    return -1;
  if (detachstate != PTHREAD_CREATE_JOINABLE && detachstate != PTHREAD_CREATE_DETACHED)
    return -1;
  attr->detach_state = detachstate;
  return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
  if(!parameters_are_valid((size_t)attr, 0) || !parameters_are_valid((size_t)detachstate, 0) || attr->initialized != 1)
  {
    return -1;
  }
  if (attr == NULL || detachstate == NULL)
    return -1;
  *detachstate = attr->detach_state;
  return 0;
}
