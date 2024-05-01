/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
#include "semaphore.h"
#include "assert.h"

#define LOOP_AMOUNT2 100        // tested with 1000 still work but take very long

sem_t sem;
pthread_mutex_t mutex;
pthread_cond_t cond;
int counter = 0;


void *increment(void *arg) {
    for (int i = 0; i < LOOP_AMOUNT2; ++i) {
        sem_wait(&sem);
        ++counter;
        assert(counter == 1);
        pthread_cond_signal(&cond);
    }
    return NULL;
}

void *decrement(void *arg) {
    for (int i = 0; i < LOOP_AMOUNT2; ++i) {
        pthread_mutex_lock(&mutex);
        while (counter == 0)
            pthread_cond_wait(&cond, &mutex);
        --counter;
        assert(counter == 0);
        pthread_mutex_unlock(&mutex);
        sem_post(&sem);
    }
    return NULL;
}

int sem2() {
    pthread_t t1, t2;

    if (sem_init(&sem, 0, 1) != 0) {
        printf("Failed to initialize semaphore\n");
        return -1;
    }

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Failed to initialize mutex\n");
        return -1;
    }

    if (pthread_cond_init(&cond, NULL) != 0) {
        printf("Failed to initialize condition variable\n");
        return -1;
    }

    if (pthread_create(&t1, NULL, increment, NULL) != 0) {
        printf("Failed to create thread\n");
        return -1;
    }

    if (pthread_create(&t2, NULL, decrement, NULL) != 0) {
        printf("Failed to create thread\n");
        return -1;
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sem_destroy(&sem);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
