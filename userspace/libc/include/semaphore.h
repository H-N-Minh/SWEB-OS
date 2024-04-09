#pragma once

#include "pthread.h"

#ifdef __cplusplus
extern "C" {
#endif

//semaphores typedefs
#ifndef SEM_T_DEFINED_
#define SEM_T_DEFINED_
typedef struct {
    pthread_mutex_t sem_mutex_;
    pthread_cond_t sem_cond_;
    size_t initialized_;
    size_t count;
} sem_t;

#endif // SEM_T_DEFINED_

extern int sem_init(sem_t *sem, int pshared, unsigned value);

extern int sem_wait(sem_t *sem);

extern int sem_trywait(sem_t *sem);

extern int sem_destroy(sem_t *sem);

extern int sem_post(sem_t *sem);

#ifdef __cplusplus
}
#endif



