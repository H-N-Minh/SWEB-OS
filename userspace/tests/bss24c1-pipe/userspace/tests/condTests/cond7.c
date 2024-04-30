#include <stdio.h>
#include <pthread.h>
#include "assert.h"


#define NUM_THREADS7 20

pthread_mutex_t mymutex7;
pthread_cond_t mycond7;

int counter7 = 0;

void* thread_func7(void* arg) {
    // Wait for the condition variable
    pthread_mutex_lock(&mymutex7);
    counter7++;
    pthread_cond_wait(&mycond7, &mymutex7);

    assert(0 && "This line should not be reached.");

    return NULL;
}

// This tests if the threads are sleeping on a cond wait and are never waken up can still be killed correctly
// when the program exits without waking them up
// also test if multiple threads go to sleep on same cond_wait
int cond7() {
    pthread_t threads[NUM_THREADS7];

    // Initialize the mutex and condition variable
    pthread_mutex_init(&mymutex7, NULL);
    pthread_cond_init(&mycond7, NULL);

    // Create threads
    for (long i = 0; i < NUM_THREADS7; i++) {
        pthread_create(&threads[i], NULL, thread_func7, (void*)i);
    }

    while (counter7 != NUM_THREADS7) {
        // Wait for all threads to reach the condition variable
    }
    pthread_mutex_lock(&mymutex7);

    // Exit without signaling or broadcasting the condition variable
    return 0;
}