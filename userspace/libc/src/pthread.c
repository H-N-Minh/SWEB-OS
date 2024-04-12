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
  return __syscall(sc_pthread_create, (size_t)thread, (size_t)attr, (size_t)start_routine, (size_t)arg, (size_t)pthread_create_wrapper);
}

//wrapper function. In pthread create
void pthread_create_wrapper(void* start_routine, void* arg)
{
  void* retval = ((void* (*)(void*))start_routine)(arg);
  pthread_exit(retval);
}


// commented out for now because this is needed only when thread has multiple stacks

// int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
//                     void *(*start_routine)(void *), void *arg)
// {
//   int retval = __syscall(sc_pthread_create, (size_t)thread, (size_t)attr, (size_t)start_routine, (size_t)arg, (size_t)pthread_create_wrapper);
//   if (!retval)
//   {
//     // if top_stack is 0, it means this is the first stack of the parent thread, so it should point to itself
//     size_t top_stack = getTopOfThisStack();
//     if(*(size_t*) top_stack == 0)
//     {
//       *(size_t*) top_stack = top_stack;
//       // these should already be 0, but just to be sure
//       top_stack -= sizeof(size_t);  *(size_t*) top_stack = 0;
//       top_stack -= sizeof(size_t);  *(size_t*) top_stack = 0;
//     }
//   }
//   return retval;
// }
// /**wrapper function. In pthread create
// // top_stack points to the top of the 1st stack of the child thread
// // Since its the first stack of new thread, it should points to itself */
// void pthread_create_wrapper(void* start_routine, void* arg, void* top_stack)
// {
//   assert(top_stack && "top_stack of Child Thread is NULL");
//   *(size_t*) top_stack = (size_t) top_stack;
//   // these should already be 0, but just to be sure:
//   top_stack -= sizeof(size_t);  *(size_t*) top_stack = 0;   // linked list for waiting threads
//   top_stack -= sizeof(size_t);  *(size_t*) top_stack = 0;   // boolean for request_to_sleep
//   void* retval = ((void* (*)(void*))start_routine)(arg);
//   pthread_exit(retval);
// }


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
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  // 1. Error handling
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    return -1;    //Error: Mutex not initalized or mutex address not valid
  }
  assert(pthread_spin_lock(&mutex->mutex_lock_) == 0);
  if(mutex->waiting_list_)
  {
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
    return -1;    //Error: Mutex is still held by some thread
  }

  // 2. Destroy the lock
  mutex->initialized_ = 0;
  assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
  assert(pthread_spin_destroy(&mutex->mutex_lock_) == 0);

  return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
  if(!parameters_are_valid((size_t)mutex, 0) || !parameters_are_valid((size_t)attr, 1) || mutex->initialized_ == MUTEX_INITALIZED)
  {
    return -1;
  }
  assert(pthread_spin_init(&mutex->mutex_lock_) == 0);

  assert(pthread_spin_lock(&mutex->mutex_lock_) == 0);
  mutex->initialized_ = MUTEX_INITALIZED;
  mutex->waiting_list_ = 0;
  assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);

  return 0;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  // 1. Error handling
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  { //printf("parameters not valid or lock not initialized\n");
    return -1;
  }
  size_t waiting_flag_address = getTopOfThisStack();
  size_t waiting_list_address = (size_t) waiting_flag_address - sizeof(size_t);
  assert(pthread_spin_lock(&mutex->mutex_lock_) == 0);
  if(mutex->waiting_list_ == waiting_list_address)    // the first thread in the waiting list is always the thread currently holding the lock
  {
    //printf("Thread is already holding lock\n");
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
    return -1;
  }

  // 2. Add itself to the waiting list (no matter if list empty or not)
  addWaiterToList((size_t*) &mutex->waiting_list_, waiting_list_address);

  // 3.1 If the thread is first in the list, that means the lock belongs to the current thread. It can just return
  //     (We define that the first in the list is always the one holding the lock)
  if (mutex->waiting_list_ == waiting_list_address)
  {
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
  }
    // 3.2 If thread is not 1st in the list, some other thread is holding the lock. It should go to sleep
  else
  {
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
    // We can safely go to sleep here because waker thread has to wait till we really sleeping
    *(size_t*) waiting_flag_address = 1;    // This tells the scheduler that this thread is waiting and can be skipped
    __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
    // From this point on is when the thread is woken up. The waker thread should have put this thread as 1st in the list by now.
    assert(mutex->waiting_list_ == waiting_list_address  && "thread is woken up but is not the first in waiting list as it should be");
  }
  return 0;
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  // 1. Error handling
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    //printf("parameters not valid or lock not initialized\n");
    return -1;
  }

  // 2.1 If the list is not empty, that means someone else is holding the lock. Return -1
  assert(pthread_spin_lock(&mutex->mutex_lock_) == 0);
  if (mutex->waiting_list_)
  {
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
    return -1;
  }
    // 2.2 If the list is empty, that means the lock is free. The current thread can take it
  else
  {
    size_t waiting_flag_address = getTopOfThisStack();
    size_t waiting_list_address = (size_t) waiting_flag_address - sizeof(size_t);
    addWaiterToList((size_t*) &mutex->waiting_list_, waiting_list_address);     // add itself to 1st spot in the list to mark that the lock belongs to this thread now
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
    return 0;
  }
}



/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  // 1. Error handling
  if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
  {
    //printf("parameter not valid or not initalized\n");
    return -1;
  }
  size_t* waiting_flag_address = (size_t*) getTopOfThisStack();
  size_t* waiting_list_address = (size_t*) ((size_t) waiting_flag_address - sizeof(size_t));
  assert(pthread_spin_lock(&mutex->mutex_lock_) == 0);
  if(mutex->waiting_list_ != waiting_list_address)
  {
    //printf("Thread is not currently holding current lock (or the lock is not held by anybody)\n");
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
    return -1;
  }

  // 2.1 If theres no other thread waiting for the lock, just clear the list and return
  if(!*waiting_list_address)
  {
    mutex->waiting_list_ = 0;
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);
  }
    // 2.2 If there are threads waiting for the lock, make it 1st in the list, then wake it up.
  else
  {
    mutex->waiting_list_ = *waiting_list_address;
    assert(pthread_spin_unlock(&mutex->mutex_lock_) == 0);    // we can unlock here because we wont be working on the list anymore
    size_t* next_waiter_flag_address = (size_t*) (*waiting_list_address + sizeof(size_t));    // the flag address of the thread we want to wake up
    wakeUpThread(next_waiter_flag_address);
  }

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
  int DEBUGMINH = 0;    // TODO: remove this debugminh
  if (DEBUGMINH == 1) {
    printf(" got into broadcast\n");
  }
  if(!parameters_are_valid((size_t)cond, 0) || !cond->initialized_)
  {
    return -1;    //Cond is Null, cond not initialized
  }
  if (DEBUGMINH == 1) {
    printf("passed parameters check\n");
  }
  while (cond->waiting_list_)   // if theres at least 1 thread in the waiting list
  {
    if (DEBUGMINH == 1) {
      printf("got into while\n");
    }
    // remove the first thread from the waiting list and wake it up
    size_t thread_to_wakeup = cond->waiting_list_;
    cond->waiting_list_ = *(size_t*) thread_to_wakeup;
    size_t* request_to_sleep = (size_t*) (thread_to_wakeup + sizeof(size_t));
    wakeUpThread(request_to_sleep);
  }
  if (DEBUGMINH == 1) {
    printf("exiting broadcast\n");
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
  size_t new_waiter_stack            = getTopOfFirstStack();
  size_t new_waiter_request_to_sleep = new_waiter_stack - 2*sizeof(size_t);   // this 2 and 3 depends on the order setup in UserThread ctor
  size_t new_waiter_list_address     = new_waiter_stack - 3*sizeof(size_t);

  // adding curent thread to the waiting list
  addWaiterToList(&cond->waiting_list_, new_waiter_list_address);

  // current thread signal that it wants to sleep
  assert(!*(size_t*) new_waiter_request_to_sleep && "threads request_to_sleep_ is already true, this should not happen");
  int rv = pthread_mutex_unlock(mutex);
  assert(rv == 0);
  *(size_t*) new_waiter_request_to_sleep = 1;      // This tells the scheduler that this thread is waiting and can be skipped
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

void addWaiterToList(size_t* waiting_list_adr, size_t new_waiter)
{
  while(*waiting_list_adr)
  {
    waiting_list_adr = (size_t*)*waiting_list_adr;
  }
  *waiting_list_adr = new_waiter;
  *(size_t*) new_waiter = 0;
}

size_t getTopOfThisStack() {
  size_t stack_variable;
  size_t top_stack = (size_t)&stack_variable - (size_t)(&stack_variable)%__PAGE_SIZE__ + __PAGE_SIZE__ - sizeof(size_t);
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

size_t getTopOfFirstStack() {
  return getTopOfThisStack();

  // commented out for now because currently each thread has only one stack
  // size_t top_current_stack = getTopOfThisStack();
  // return *(size_t*) top_current_stack;
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




// from stefie
// int pthread_mutex_lock(pthread_mutex_t *mutex)
// {
//   if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
//   {
//     //printf("parameters not valid or lock not initialized\n");
//     return -1;
//   }
//   int rv = pthread_spin_lock(&mutex->mutex_lock_);
//   assert(rv == 0);
//   size_t stack_variable;
//   size_t* waiting_list_address = (size_t*)((size_t)&stack_variable + 4080 - (size_t)(&stack_variable)%4096);
//   size_t* waiting_flag_address = (size_t*)((size_t)&stack_variable + 4088 - (size_t)(&stack_variable)%4096);
//   if(mutex->held_by_ == waiting_list_address)
//   {
//     //printf("Thread is already holding lock\n");
//     int rv = pthread_spin_unlock(&mutex->mutex_lock_);
//     assert(rv == 0);
//     return -1;
//   }
//   int counter1 = 0;
//   while (mutex->locked_)
//   {
//     counter1++;
//     //*waiting_list_address =(size_t)&mutex->waiting_list_;
//     //mutex->waiting_list_ = waiting_list_address;
//     size_t* next_element = (size_t*)&mutex->waiting_list_;
//     int added_to_waiting_list = 0;
//     while(!added_to_waiting_list)
//     {
//       if(*next_element != NULL)
//       {
//         next_element = (size_t*)(*next_element);
//       }
//       else
//       {
//         *next_element =  (size_t)waiting_list_address;
//         *waiting_list_address = NULL;
//         added_to_waiting_list = 1;
//         break;
//       }
//     }
//     //*waiting_flag_address = 1;
//     //pthread_spin_unlock(&mutex->mutex_lock_);
//     mutex->mutex_lock_.held_by_ = 0;
//     asm("xchg %0,%1"
//           : "=r" (*waiting_flag_address)
//           : "m" (mutex->mutex_lock_.locked_), "0" (*waiting_flag_address)
//           : "memory");
//     __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
//     //pthread_spin_lock(&mutex->mutex_lock_);
//   }
//   // if(counter1 != 0 && counter1 != 1)
//   //   assert(0 && "yield more than once");
//     //printf("counterabc %d\n", counter1);
//   mutex->locked_ = 1;
//   mutex->held_by_ = waiting_list_address;
//   rv= pthread_spin_unlock(&mutex->mutex_lock_);
//   assert(rv == 0);
//   //assert(mutex->held_by_ == waiting_list_address && "Held by differs from thread currently holding the lock");
//   return 0;
// }
// int pthread_mutex_trylock(pthread_mutex_t *mutex)
// {
//   if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
//   {
//     //printf("parameters not valid or lock not initialized\n");
//     return -1;
//   }
//   int rv = pthread_spin_lock(&mutex->mutex_lock_);
//   assert(rv == 0);
//   size_t stack_variable;
//   size_t* waiting_list_address = (size_t*)((size_t)&stack_variable + 4080 - (size_t)(&stack_variable)%4096);
//   if(mutex->held_by_ == waiting_list_address)
//   {
//     //printf("Thread is already holding lock\n");
//     int rv = pthread_spin_unlock(&mutex->mutex_lock_);
//     assert(rv == 0);
//     return -1;
//   }
//   if(!mutex->locked_)
//   {
//     mutex->locked_ = 1;
//     mutex->held_by_ = waiting_list_address;
//     int rv = pthread_spin_unlock(&mutex->mutex_lock_);
//     assert(rv == 0);
//   }
//   else
//   {
//     int rv = pthread_spin_unlock(&mutex->mutex_lock_);
//     assert(rv == 0);
//     return -1;
//   }
//   return 0;
// }
// int pthread_mutex_unlock(pthread_mutex_t *mutex)
// {
//   if(!parameters_are_valid((size_t)mutex, 0) || mutex->initialized_ != MUTEX_INITALIZED)
//   {
//     //printf("parameter not valid or not initalized\n");
//     return -1;
//   }
//   int rv = pthread_spin_lock(&mutex->mutex_lock_);
//   assert(rv == 0);
//   if(mutex->locked_ == 0)
//   {
//     //printf("lock that you want to unlock is not locked\n");
//     rv = pthread_spin_unlock(&mutex->mutex_lock_);
//     assert(rv == 0);
//     return -1;
//   }
//   size_t stack_variable;
//   size_t* waiting_list_address = (size_t*)((size_t)&stack_variable + 4088 - 8 - (size_t)(&stack_variable)%4096);
//   if(mutex->held_by_ != waiting_list_address)
//   {
//    // printf("Thread does not hold current lock\n");
//     int rv = pthread_spin_unlock(&mutex->mutex_lock_);
//     assert(rv == 0);
//     return -1;
//   }
//   mutex->locked_ = 0;
//   mutex->held_by_ = 0;
//   if(mutex->waiting_list_ != NULL)
//   {
//     size_t* thread_to_wake_up_ptr = ((size_t*)((size_t)mutex->waiting_list_ + (size_t)8));
//     mutex->waiting_list_ = (size_t*)*mutex->waiting_list_;
//     *thread_to_wake_up_ptr = 0;
//     mutex->mutex_lock_.held_by_ = (size_t*)((size_t)thread_to_wake_up_ptr + (size_t)8);
//   }
//   else
//   {
//     rv = pthread_spin_unlock(&mutex->mutex_lock_);
//     assert(rv == 0);
//   }
//   return 0;
// }

