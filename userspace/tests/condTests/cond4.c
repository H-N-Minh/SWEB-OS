/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include <stdio.h>
#include <pthread.h>

// NOTE: for normal pthread this number can reach 10k threads and still work
#define NUM_THREADS_4 200


pthread_mutex_t mymutex4 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mycond4 = PTHREAD_COND_INITIALIZER;
int shared_variable_4 = 0;

void* threadFunc4(void* arg) {
    pthread_mutex_lock(&mymutex4);
    // printf("Thread %d: Acquired mutex\n", *((int*)arg));
    while (shared_variable_4 == 0) {
        // printf("Thread %d: Waiting for condition variable\n", *((int*)arg));
        pthread_cond_wait(&mycond4, &mymutex4);
    }
    pthread_mutex_unlock(&mymutex4);
    // printf("Thread %d: Released mutex\n", *((int*)arg));
    return NULL;
}


int cond4() {
    pthread_t threads[NUM_THREADS_4];
    int threadIds[NUM_THREADS_4];

    for (int i = 0; i < NUM_THREADS_4; i++) {
        threadIds[i] = i + 1;
        pthread_create(&threads[i], NULL, threadFunc4, &threadIds[i]);
        // printf("Created thread %d\n", threadIds[i]);
    }

    for (size_t i = 0; i < 200000000; i++)       // small delay to make sure all threads are waiting
    {
        /* code */
    }
    

    pthread_mutex_lock(&mymutex4);
    // printf("Main thread: Acquired mutex\n");
    shared_variable_4 = 1;
    // printf("Main thread: Set shared variable to 1\n");
    pthread_cond_broadcast(&mycond4);
    // printf("Main thread: Signaled condition variable\n");
    pthread_mutex_unlock(&mymutex4);
    // printf("Main thread: Released mutex\n");

    for (int i = 0; i < NUM_THREADS_4; i++) {
        pthread_join(threads[i], NULL);
        // printf("Joined thread %d\n", threadIds[i]);
    }
    // printf("All threads joined\n");

    return 0;
}