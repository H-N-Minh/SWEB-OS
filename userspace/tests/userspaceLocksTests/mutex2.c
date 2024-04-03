#include "stdio.h"
#include "pthread.h"
#include "assert.h"

#define NUM_THREADS 100
#define MAX_COUNT   100000

int mutex_counter = 0;
pthread_mutex_t mutex;

void increment_mutex_counter()
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        pthread_mutex_lock(&mutex);
        int current_mutex_counter = mutex_counter;
        current_mutex_counter++;
        

        for(int i = 0; i < 100; i++){}             //introduces small delay, which increases race condition

        mutex_counter = current_mutex_counter;
        pthread_mutex_unlock(&mutex);
    }
    //printf("mutex_counter value: %d\n", mutex_counter);
}

int mutex2() {
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex, 0);

    for (int t = 0; t < NUM_THREADS; t++)
    {
        pthread_create(&threads[t], NULL, (void* (*)(void*))increment_mutex_counter, NULL);
    }

    for (int t = 0; t < NUM_THREADS; t++)
    {
        pthread_join(threads[t], NULL);
    }

    assert(mutex_counter == NUM_THREADS * MAX_COUNT);
    printf("mutex2 successful!\n");
    //printf("Final mutex_counter value: %d\n", mutex_counter);

    return 0;
}