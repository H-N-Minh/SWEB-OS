/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "stdio.h"
#include "pthread.h"
#include "assert.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int flag = 0;

int step = 0;   // each step is fixed, if the step is not executed in order, that means cond not working

void* thread_func(void* arg)
{
    step++;
    // printf("Step %d.\n", step);
    assert(step == 1 && "Step 1 failed!");

    int rv = pthread_mutex_lock(&mutex);
    assert(rv == 0);

    step++;
    // printf("Step %d.\n", step);
    assert(step == 2 && "Step 2 failed!");

    while (flag == 0) {
        step++;
        // printf("Step %d.\n", step);
        assert(step == 3 && "Step 3 failed!");
        pthread_cond_wait(&cond, &mutex);
    }

    step++;
    // printf("Step %d.\n", step);
    assert(step == 7 && "Step 7 failed!");

    rv = pthread_mutex_unlock(&mutex);
    assert(rv == 0);
    return NULL;
}

int main()
{
    pthread_t thread;

    pthread_create(&thread, NULL, thread_func, NULL);

    for (int i = 0; i < 200000000; i++) {   //5s delay
    // Do nothing
    }

    step++;
    // printf("Step %d.\n", step);
    assert(step == 4 && "Step 4 failed!");
    int rv = pthread_mutex_lock(&mutex);
    assert(rv == 0);

    step++;
    // printf("Step %d.\n", step);
    assert(step == 5 && "Step 5 failed!");
    flag = 1;

    rv = pthread_cond_signal(&cond);
    assert(rv == 0);
    step++;
    // printf("Step %d.\n", step);
    assert(step == 6 && "Step 6 failed!");

    rv = pthread_mutex_unlock(&mutex);
    assert(rv == 0);
    pthread_join(thread, NULL);

    step++;
    // printf("Step %d.\n", step);
    assert(step == 8 && "Step 8 failed!");
    return 0;
}