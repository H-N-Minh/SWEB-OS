/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "assert.h"

#define NUM_THREADS3 20     // shouldnt be too high because theres no locking so theres race condition here

sem_t mysem3;
int mycounter3 = 0;

void* thread_function3(void* arg) {
    mycounter3++;
    sem_wait(&mysem3);
    assert(0 && "This line should not be reached.");

    return 0;
}

int main() {
    sem_init(&mysem3, 0, 0);

    pthread_t threads[NUM_THREADS3];
    for (long i = 0; i < NUM_THREADS3; i++) {
        pthread_create(&threads[i], NULL, thread_function3, (void*)i);
    }

    // Wait for a while before exiting
    sleep(1);
    assert(mycounter3 == NUM_THREADS3  && "this might fail if num threads is too high, which causes race condition");

    return 0;
}
