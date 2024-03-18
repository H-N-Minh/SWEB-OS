#include "stdio.h"
#include "pthread.h"

#define NUM_THREADS 10
#define MAX_COUNT   100000


int pthread_spin_init1(unsigned int *lock, int pshared)
{
    if (lock == ((void*)0))
        return 1; //error
    *lock = 0; //initialize the spinlock to an unlocked state
    return 0; //success
}

int pthread_spin_lock1(unsigned int *lock)
{
    int old_val = 1;
    do {
        asm("xchg %0,%1"
                : "=r" (old_val)
                : "m" (*lock), "0" (old_val)
                : "memory");
    } while (old_val);
    return 0; // Success
}

int pthread_spin_unlock1(unsigned int *lock)
{
    *lock = 0; //release the lock
    return 0; //
}

int counter = 0;
unsigned int lock;

void *increment_counter() {
    for (int i = 0; i < MAX_COUNT; i++) {
        pthread_spin_lock1(&lock);
        counter++;
        pthread_spin_unlock1(&lock);
    }
    printf("Counter value: %d\n", counter);
    // pthread_exit(NULL);
    return 0;
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_spin_init1(&lock, 0);

    for (int t = 0; t < NUM_THREADS; t++)
    {
        pthread_create(&threads[t], NULL, increment_counter, NULL);
    }

    printf("Final Counter value: %d\n", counter);

    while (1)
    {

    }

    return 0;
}