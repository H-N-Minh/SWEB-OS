#pragma once

#include "types.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"

#define USER_BREAK 0x0000800000000000ULL

#ifdef __cplusplus
extern "C" {
#endif

//pthread typedefs
typedef size_t pthread_t;
typedef unsigned int pthread_attr_t;

//pthread spinlock
struct pthread_spinlock_struct { 
    size_t  locked_; 
    size_t initialized_;
    void* held_by_;
}; 

typedef struct pthread_spinlock_struct pthread_spinlock_t;



//pthread mutex
struct pthread_mutex_struct { 
    size_t initialized_;
    size_t *waiting_list_;
    pthread_spinlock_t mutex_lock_;
    size_t *held_by_;
    size_t locked_;
}; 



#define MUTEX_INITALIZED 12341234
#define SPINLOCK_INITALIZED 14243444

#define PTHREAD_SPIN_INITIALIZER { .locked_ = 0, .initialized_= SPINLOCK_INITALIZED, .held_by_ = 0 }
#define PTHREAD_MUTEX_INITIALIZER {.initialized_= MUTEX_INITALIZED, .waiting_list_ = 0, .mutex_lock_ = PTHREAD_SPIN_INITIALIZER }
#define PTHREAD_COND_INITIALIZER { .initialized_= 1, .waiting_list_ = 0 }

typedef struct pthread_mutex_struct pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;



//pthread cond typedefs
typedef struct pthread_cond_struct
{
  size_t initialized_;
  size_t waiting_list_;     // pointer to linked list of waiting threads
} pthread_cond_t;
typedef unsigned int pthread_condattr_t;

enum CancelState {PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE};
enum CancelType {PTHREAD_CANCEL_DEFERRED = 2, PTHREAD_CANCEL_ASYNCHRONOUS = 3};


extern int pthread_create(pthread_t *thread,
         const pthread_attr_t *attr, void *(*start_routine)(void *),
         void *arg);


/**pthread_create_wrapper is the wrapper that we need to run first
//the wrapper will take 2 parameter
//first one for the function that we want to run
//second one is the parameter of the function */
void pthread_create_wrapper(void* start_routine, void* arg);

extern void pthread_exit(void *value_ptr);

extern int pthread_cancel(pthread_t thread);

extern int pthread_join(pthread_t thread, void **value_ptr);

extern int pthread_detach(pthread_t thread);

extern int pthread_mutex_init(pthread_mutex_t *mutex,
                              const pthread_mutexattr_t *attr);

extern int pthread_mutex_destroy(pthread_mutex_t *mutex);

extern int pthread_mutex_lock(pthread_mutex_t *mutex);

extern int pthread_mutex_trylock(pthread_mutex_t *mutex);

extern int pthread_mutex_unlock(pthread_mutex_t *mutex);

extern int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);

extern int pthread_cond_destroy(pthread_cond_t *cond);

extern int pthread_cond_signal(pthread_cond_t *cond);

extern int pthread_cond_broadcast(pthread_cond_t *cond);

extern int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

extern int pthread_setcancelstate(int state, int *oldstate);

extern int pthread_setcanceltype(int type, int *oldtype);

extern void pthread_testcancel(void);

extern int get_thread_count(void);


extern int pthread_spin_destroy(pthread_spinlock_t *lock);

extern int pthread_spin_init(pthread_spinlock_t *lock, int pshared);

extern int pthread_spin_lock(pthread_spinlock_t *lock);

extern int pthread_spin_trylock(pthread_spinlock_t *lock);

extern int pthread_spin_unlock(pthread_spinlock_t *lock);


extern int parameters_are_valid(size_t ptr, int allowed_to_be_null);

extern void print_waiting_list(size_t* waiting_list, int before);

/**
 * return the address of the top of the current stack. 
 * @return non null pointer
*/
extern size_t getTopOfThisStack();

/**
 * return the address of the top of the first stack. 
 * @return non null pointer
*/
extern size_t getTopOfFirstStack();

/**
 * Add the new_waiter to the last of the waiting_list
 * NOTE: lock the list before calling this function
 * @param waiting_list_adr the ADDRESS of the waiting_list (&waiting_list_)
*/
extern void addWaiterToList(size_t* waiting_list_adr, size_t new_waiter);

/**
 * Wake up a thread by setting its request_to_sleep to 0
 * if the thread is not sleeping yet, then wait until it sleep then wake it up
 * Since this use spinlock to wait, the currentThread will be blocked until the other thread wake up
 * @param request_to_sleep the address of flag that is used to tell Scheduler to skip this thread
*/
void wakeUpThread(size_t* request_to_sleep);

#ifdef __cplusplus
}
#endif


