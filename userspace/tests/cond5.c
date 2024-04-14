/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

pthread_mutex_t mymutex5;

int main() {
    pthread_mutex_init(&mymutex5, NULL);
    pthread_cond_t cond;

    // Try to use the condition variable before it's initialized
    assert(pthread_cond_wait(&cond, &mymutex5) == -1);
    assert(pthread_cond_broadcast(&cond) == -1);
    assert(pthread_cond_signal(&cond) == -1);
    assert(pthread_cond_destroy(&cond) == -1);

    // Try to use NULL as parameter
    assert(pthread_cond_init(NULL, NULL) == -1);

    // Initialize the condition variable correctly to really only test the NULL parameter
    assert(pthread_cond_init(&cond, NULL) == 0);

    assert(pthread_cond_wait(NULL, &mymutex5) == -1);
    assert(pthread_cond_wait(&cond, NULL) == -1);
    assert(pthread_cond_broadcast(NULL) == -1);
    assert(pthread_cond_signal(NULL) == -1);
    assert(pthread_cond_destroy(NULL) == -1);

    // Destroy the condition variable for next test
    assert(pthread_cond_destroy(&cond) == 0);

    // Try to use destroyed condition variable
    assert(pthread_cond_wait(&cond, &mymutex5) == -1);
    assert(pthread_cond_broadcast(&cond) == -1);
    assert(pthread_cond_signal(&cond) == -1);
    assert(pthread_cond_init(&cond, NULL) == 0);        // init after destroy should be fine

    assert(pthread_cond_destroy(&cond) == 0);
    pthread_mutex_destroy(&mymutex5);
    return 0;
}


