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

//pthread mutex typedefs
// typedef unsigned int pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;

typedef struct pthread_spinlock_struct { 
    size_t  locked_; 
    size_t initialized_;
    void* held_by_;
} pthread_spinlock_t; 


// TODO: MUTEX is only copying spinlock for now. Delete this struct
typedef struct pthread_mutex_struct { 
    size_t  locked_; 
    size_t initialized_;
    void* held_by_;
} pthread_mutex_t; 

#define SPINLOCK_INITALIZED 14243444
#define PTHREAD_SPIN_INITIALIZER { .locked_ = 0, .initialized_= SPINLOCK_INITALIZED, .held_by_ = 0 }
#define PTHREAD_COND_INITIALIZER { .initialized_= 1, .waiting_list_ = 0 }

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
void pthread_create_wrapper(void* start_routine, void* arg, void* top_stack);

extern void pthread_exit(void *value_ptr);

extern int pthread_cancel(pthread_t thread);

extern int pthread_join(pthread_t thread, void **value_ptr);

extern int pthread_detach(pthread_t thread);

extern int pthread_mutex_init(pthread_mutex_t *mutex,
                              const pthread_mutexattr_t *attr);

extern int pthread_mutex_destroy(pthread_mutex_t *mutex);

extern int pthread_mutex_lock(pthread_mutex_t *mutex);

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


/**
 * @return loop through linked list and get the pointer of last thread that is in the waiting_list_ of the given cond
 * @return 0 if the list is empty
*/
extern size_t getLastCondWaiter(pthread_cond_t* cond);

/**
 * return the address of the top of the current stack. 
 * @return non null pointer
*/
extern size_t* getTopOfThisStack();

/**
 * return the address of the top of the first stack. 
 * @return non null pointer
*/
extern size_t* getTopOfFirstStack();

#ifdef __cplusplus
}
#endif


