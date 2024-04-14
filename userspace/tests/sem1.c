/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: true
*/

#include "stdio.h"
#include <pthread.h>
#include <semaphore.h>
#include "assert.h"

#define DEBUG1 1

sem_t mysem1;
size_t step1 = 0;

void printStep(size_t step) {
    #if DEBUG1
        // printf("Step: %zu\n", step);
    #endif
}

void* thread_function1(void* arg) {
    step1++;
    printStep(step1);
    assert(step1 == 1 && "Failed at step 1\n");

    sem_wait(&mysem1);
    step1++;
    printStep(step1);
    assert(step1 == 2 && "Failed at step 2\n");

    // printf("count is %zu\n", mysem1.count_);
    // printf("Thread 1 before pausing\n");
    for (size_t i = 0; i < 200000000; i++) {
        /* code */
    }
    // printf("Thread 1 after pausing\n");

    step1++;
    printStep(step1);
    assert(step1 == 3 && "Failed at step 3\n");

    sem_post(&mysem1);

    for (size_t i = 0; i < 100000000; i++) {
        /* code */
    }

    sem_wait(&mysem1);

    return NULL;
}

void* thread_function11(void* arg) {
    for (size_t i = 0; i < 100000000; i++) {
        /* code */
    }

    // printf("Thread 2 before calling sem wait\n");
    sem_wait(&mysem1);
    // printf("Thread 2 after calling sem wait\n");
    step1++;
    printStep(step1);
    assert(step1 == 4 && "Failed at step 4\n");

    for (size_t i = 0; i < 100000000; i++) {
        /* code */
    }

    step1++;
    printStep(step1);
    assert(step1 == 5 && "Failed at step 5\n");

    sem_post(&mysem1);

    return NULL;
}

int main() {
    sem_init(&mysem1, 0, 1);

    pthread_t thread1, thread2;
    int thread_id1 = 1, thread_id2 = 2;
    pthread_create(&thread1, NULL, thread_function1, &thread_id1);
    pthread_create(&thread2, NULL, thread_function11, &thread_id2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    sem_destroy(&mysem1);

    return 0;
}