#include "stdio.h"
#include "pthread.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int flag = 0;

void* thread_func(void* arg)
{
    printf("2. thread locking...\n");
    pthread_mutex_lock(&mutex);
    printf("2. thread got the lock...\n");

    while (flag == 0) {
        printf("2. Waiting for signal...\n");
        pthread_cond_wait(&cond, &mutex);
    }

    printf("2. thread wake up!\n");

    pthread_mutex_unlock(&mutex);

    return NULL;
}

int cond1()
{
    pthread_t thread;

    pthread_create(&thread, NULL, thread_func, NULL);

    for (int i = 0; i < 100000000; i++) {
    // Do nothing
    }

    
    printf("1. thread locking...\n");
    pthread_mutex_lock(&mutex);
    printf("1. thread got the lock...\n");
    flag = 1;
    pthread_cond_signal(&cond);

    printf("1. flag changed an signaled...\n");
    pthread_mutex_unlock(&mutex);

    pthread_join(thread, NULL);

    return 0;
}