#include "stdio.h"
#include "pthread.h"

#define NUM_THREADS_4 10


pthread_mutex_t mymutex4 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mycond4 = PTHREAD_COND_INITIALIZER;
int shared_variable_4 = 0;

void* threadFunc4(void* arg) {
    pthread_mutex_lock(&mymutex4);
    while (shared_variable_4 == 0) {
        pthread_cond_wait(&mycond4, &mymutex4);
    }
    pthread_mutex_unlock(&mymutex4);
    return NULL;
}


int cond4() {
    pthread_t threads[NUM_THREADS_4];
    int threadIds[NUM_THREADS_4];

    for (int i = 0; i < NUM_THREADS_4; i++) {
        threadIds[i] = i + 1;
        pthread_create(&threads[i], NULL, threadFunc4, &threadIds[i]);
    }

    pthread_mutex_lock(&mymutex4);
    shared_variable_4 = 1;
    pthread_cond_broadcast(&mycond4);
    pthread_mutex_unlock(&mymutex4);

    for (int i = 0; i < NUM_THREADS_4; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}