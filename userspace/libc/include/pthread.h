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
typedef unsigned int pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;

typedef struct pthread_spinlock_struct { 
    size_t  locked_; 
    size_t initialized_;
    void* held_by_;
} pthread_spinlock_t; 


//pthread cond typedefs
typedef unsigned int pthread_cond_t;
typedef unsigned int pthread_condattr_t;

enum CancelState {PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DISABLE};
enum CancelType {PTHREAD_CANCEL_DEFERRED = 2, PTHREAD_CANCEL_ASYNCHRONOUS = 3};


/**
 * return the address of the top of the current stack
*/
extern size_t* getTopOfThisStack();

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

#ifdef __cplusplus
}
#endif


