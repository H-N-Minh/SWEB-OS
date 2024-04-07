#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

#define THREAD_COUNT 100
#define MAX_COUNT   5000

long mutex_counter_1 = 0;
long mutex_counter_2 = 0;

pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2  = PTHREAD_MUTEX_INITIALIZER;

int increment_mutex_counter_1(void* thread_id)
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        int rv_lock = pthread_mutex_lock(&mutex_1);
        assert(rv_lock == 0);
        long temp = mutex_counter_1 + 1;
        if(i % 100 == 0)
        {
          pthread_testcancel();
        }
        mutex_counter_1 = temp;
        int rv_unlock = pthread_mutex_unlock(&mutex_1);
        assert(rv_unlock == 0);

        
       // printf("Counter 1 %ld with thread %ld.\n",mutex_counter_1, (long)thread_id);

    }
    return 42;
}

int increment_mutex_counter_2(void* thread_id)
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        int rv_lock = pthread_mutex_lock(&mutex_2);
        assert(rv_lock == 0);
        long temp = mutex_counter_2 + 1;
        if(i % 100 == 0)
        {
          pthread_testcancel();
        }
        mutex_counter_2 = temp;
        int rv_unlock = pthread_mutex_unlock(&mutex_2);
        assert(rv_unlock == 0);

       // printf("Counter 2 %ld with thread %ld.\n",mutex_counter_2, (long)thread_id);
    }
    return 42;
}


int mutex3() {
    pthread_t threads1[THREAD_COUNT];
    pthread_t threads2[THREAD_COUNT];
    int rv_init_1 = pthread_mutex_init(&mutex_1, 0);
    assert(rv_init_1 == 0);

    for (long t = 0; t < THREAD_COUNT; t++)
    {
        int rv_create = pthread_create(&threads1[t], NULL, (void* (*)(void*))increment_mutex_counter_1, (void*)t);
        assert(rv_create == 0);
        rv_create = pthread_create(&threads2[t], NULL, (void* (*)(void*))increment_mutex_counter_2, (void*)(t + THREAD_COUNT));
        assert(rv_create == 0);
    }


    for (long t = 0; t < THREAD_COUNT; t++)
    {
        int rv_join = pthread_join(threads1[t], NULL);
        assert(rv_join == 0);

        rv_join = pthread_join(threads2[t], NULL);
        assert(rv_join == 0);
    }

    assert(mutex_counter_1 == THREAD_COUNT * MAX_COUNT);
    assert(mutex_counter_2 == THREAD_COUNT * MAX_COUNT);

    int rv_destroy = pthread_mutex_destroy(&mutex_1);
    assert(rv_destroy == 0);

    rv_destroy = pthread_mutex_destroy(&mutex_2);
    assert(rv_destroy == 0);
    printf("mutex3 successful!\n");
    //printf("Final mutex_counter value: %d\n", mutex_counter);

    return 0;
}

