#ifndef MINHCOND_H
#define MINHCOND_H

#include <stdio.h>
#include <pthread.h>

typedef struct {
    int value;
    pthread_mutex_t mutex;
} Minh_pthread_cond_t;

void Minh_pthread_cond_init(Minh_pthread_cond_t* cv, const pthread_condattr_t *attr);
void Minh_pthread_cond_wait(Minh_pthread_cond_t* cv, pthread_mutex_t* given_mutex);
void Minh_pthread_cond_signal(Minh_pthread_cond_t* cv);
void Minh_pthread_cond_destroy(Minh_pthread_cond_t* cv);

#endif // MINHCOND_H
