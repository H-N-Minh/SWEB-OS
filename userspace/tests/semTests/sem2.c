#include "stdio.h"
#include "pthread.h"
#include "assert.h"

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mycond2;

#define WAIT_TIME_COND2 200000000

int flag2 = 0;
int step2 = 0;   // each step is fixed, if the step2 is not executed in order, that means cond not working


// Test that both threads use cond_wait at same time. First child waits for parent's signal,
//  then parent waits for child's signal
void* thread_func2(void* arg)
{
    step2++;
    assert(step2 == 1 && "step 1 failed!");

    pthread_mutex_lock(&mutex2);

    step2++;
    assert(step2 == 2 && "step 2 failed!");

    while (flag2 == 0) {
        step2++;
        assert(step2 == 3 && "step 3 failed!");
        pthread_cond_wait(&mycond2, &mutex2);
    }

    step2++;
    assert(step2 == 7 && "step 7 failed!");

    pthread_mutex_unlock(&mutex2);

    for (int i = 0; i < WAIT_TIME_COND2; i++) {   //5s delay
        // Do nothing
    }

    pthread_mutex_lock(&mutex2);

    step2++;
    assert(step2 == 8 && "step 8 failed!");

    flag2 = 2;
    pthread_cond_signal(&mycond2);
    step2++;
    assert(step2 == 9 && "step 9 failed!");

    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int cond2()
{
    pthread_t thread;

    pthread_cond_init(&mycond2, NULL);

    pthread_create(&thread, NULL, thread_func2, NULL);

    for (int i = 0; i < WAIT_TIME_COND2; i++) {   //5s delay
        // Do nothing
    }

    step2++;
    assert(step2 == 4 && "step 4 failed!");
    pthread_mutex_lock(&mutex2);

    step2++;
    assert(step2 == 5 && "step 5 failed!");
    flag2 = 1;

    pthread_cond_signal(&mycond2);

    while (flag2 == 1) {
        step2++;
        assert(step2 == 6 && "step 6 failed!");
        pthread_cond_wait(&mycond2, &mutex2);
    }

    step2++;
    assert(step2 == 10 && "step 10 failed!");

    pthread_mutex_unlock(&mutex2);
    pthread_join(thread, NULL);

    step2++;
    assert(step2 == 11 && "step 11 failed!");
    return 0;
}