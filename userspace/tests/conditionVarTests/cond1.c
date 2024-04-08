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
    assert(step == 1 && "Step 1 failed!");

    pthread_mutex_lock(&mutex);

    step++;
    assert(step == 2 && "Step 2 failed!");

    while (flag == 0) {
        step++;
        assert(step == 3 && "Step 3 failed!");
        pthread_cond_wait(&cond, &mutex);
    }

    step++;
    assert(step == 7 && "Step 7 failed!");

    pthread_mutex_unlock(&mutex);
    return NULL;
}

int cond1()
{
    pthread_t thread;

    pthread_create(&thread, NULL, thread_func, NULL);

    for (int i = 0; i < 800000000; i++) {   //5s delay
    // Do nothing
    }

    
    step++;
    assert(step == 4 && "Step 4 failed!");
    pthread_mutex_lock(&mutex);

    step++;
    assert(step == 5 && "Step 5 failed!");
    flag = 1;

    pthread_cond_signal(&cond);
    step++;
    assert(step == 6 && "Step 6 failed!");

    pthread_mutex_unlock(&mutex);
    pthread_join(thread, NULL);

    step++;
    assert(step == 8 && "Step 8 failed!");
    return 0;
}