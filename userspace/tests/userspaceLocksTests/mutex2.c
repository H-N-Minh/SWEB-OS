#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

#define NUM_THREADS 100
#define MAX_COUNT   10000 //0

long mutex_counter = 0;
pthread_mutex_t mutex;

int increment_mutex_counter(void* thread_id)
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        int rv_lock = pthread_mutex_lock(&mutex);
        assert(rv_lock == 0);
        int current_mutex_counter = mutex_counter;
        current_mutex_counter++;
        
        for(int i = 0; i < 100; i++){}             //introduces small delay, which increases race condition

        mutex_counter = current_mutex_counter;
        int rv_unlock = pthread_mutex_unlock(&mutex);
        assert(rv_unlock == 0);

    }
    return 123;
}

int mutex2() {
    pthread_t threads[NUM_THREADS];
    int rv_init = pthread_mutex_init(&mutex, 0);
    assert(rv_init == 0);

    for (long t = 0; t < NUM_THREADS; t++)
    {
        int rv_create = pthread_create(&threads[t], NULL, (void* (*)(void*))increment_mutex_counter, (void*)t);
        assert(rv_create == 0);
    }

    for (long t = 0; t < NUM_THREADS; t++)
    {
        int rv_join = pthread_join(threads[t], NULL);
        assert(rv_join == 0);
    }

    assert(mutex_counter == NUM_THREADS * MAX_COUNT);
    int rv_destroy = pthread_mutex_destroy(&mutex);
    assert(rv_destroy == 0);
    printf("mutex2 successful!\n");

    return 0;
}


