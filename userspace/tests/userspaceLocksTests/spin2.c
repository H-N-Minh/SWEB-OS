#include "stdio.h"
#include "pthread.h"
#include "assert.h"

#define NUM_THREADS 100
#define MAX_COUNT   100000

int counter = 0;
pthread_spinlock_t lock;

void increment_counter()
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        pthread_spin_lock(&lock);
        int current_counter = counter;
        current_counter++;
        

        for(int i = 0; i < 100; i++){}             //introduces small delay

        counter = current_counter;
        pthread_spin_unlock(&lock);
    }
    //printf("Counter value: %d\n", counter);
}

int spin2() {
    pthread_t threads[NUM_THREADS];
    pthread_spin_init(&lock, 0);

    for (int t = 0; t < NUM_THREADS; t++)
    {
        pthread_create(&threads[t], NULL, (void* (*)(void*))increment_counter, NULL);
    }

    for (int t = 0; t < NUM_THREADS; t++)
    {
        pthread_join(threads[t], NULL);
    }

    assert(counter == NUM_THREADS * MAX_COUNT);
    printf("spin2 successful!\n");
    //printf("Final Counter value: %d\n", counter);

    return 0;
}