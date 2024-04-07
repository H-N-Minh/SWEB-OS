#include <stdio.h>
#include <pthread.h>
#include "Minhcond.h"

pthread_mutex_t mutex;
Minh_pthread_cond_t cond;
int flag = 0;

void* thread_func() {
    pthread_mutex_lock(&mutex);
    while (flag == 0) {
        Minh_pthread_cond_wait(&cond, &mutex);
        printf("this should be printed only once!\n");
    }
    printf("Thread woke up!\n");
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_mutex_init(&mutex, NULL);
    Minh_pthread_cond_init(&cond, NULL);

    pthread_create(&thread, NULL, thread_func, NULL);

    // Simulating some work
    printf("Main thread is going to sleep, other thread must wait for cond...\n");
    sleep(3);
    printf("Main thread woke up, other thread must not wake up yet\n");
    pthread_mutex_lock(&mutex);
    flag = 1;
    Minh_pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_join(thread, NULL);

    pthread_mutex_destroy(&mutex);
    Minh_pthread_cond_destroy(&cond);

    return 0;
}