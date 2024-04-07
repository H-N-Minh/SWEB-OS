#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"

#define THREAD_COUNT 100
#define MAX_COUNT   5000

long spin_counter_1 = 0;
long spin_counter_2 = 0;

pthread_spinlock_t spin_1;
pthread_spinlock_t spin_2;

int increment_spin_counter_1(void* thread_id)
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        int rv_lock = pthread_spin_lock(&spin_1);
        assert(rv_lock == 0);
        long temp = spin_counter_1 + 1;
        if(i % 100 == 0)
        {
          pthread_testcancel();
        }
        spin_counter_1 = temp;
        int rv_unlock = pthread_spin_unlock(&spin_1);
        assert(rv_unlock == 0);

        
       // printf("Counter 1 %ld with thread %ld.\n",spin_counter_1, (long)thread_id);

    }
    return 42;
}

int increment_spin_counter_2(void* thread_id)
{
    for (int i = 0; i < MAX_COUNT; i++)
    {
        int rv_lock = pthread_spin_lock(&spin_2);
        assert(rv_lock == 0);
        long temp = spin_counter_2 + 1;
        if(i % 100 == 0)
        {
          pthread_testcancel();
        }
        spin_counter_2 = temp;
        int rv_unlock = pthread_spin_unlock(&spin_2);
        assert(rv_unlock == 0);

       // printf("Counter 2 %ld with thread %ld.\n",spin_counter_2, (long)thread_id);
    }
    return 42;
}


int spin3() {
    pthread_t threads1[THREAD_COUNT];
    pthread_t threads2[THREAD_COUNT];
    int rv_init_1 = pthread_spin_init(&spin_1, 0);
    assert(rv_init_1 == 0);
    int rv_init_2 = pthread_spin_init(&spin_2, 0);
    assert(rv_init_2 == 0);

    for (long t = 0; t < THREAD_COUNT; t++)
    {
        int rv_create = pthread_create(&threads1[t], NULL, (void* (*)(void*))increment_spin_counter_1, (void*)t);
        assert(rv_create == 0);
        rv_create = pthread_create(&threads2[t], NULL, (void* (*)(void*))increment_spin_counter_2, (void*)(t + THREAD_COUNT));
        assert(rv_create == 0);
    }


    for (long t = 0; t < THREAD_COUNT; t++)
    {
        int rv_join = pthread_join(threads1[t], NULL);
        assert(rv_join == 0);

        rv_join = pthread_join(threads2[t], NULL);
        assert(rv_join == 0);
    }

    assert(spin_counter_1 == THREAD_COUNT * MAX_COUNT);
    assert(spin_counter_2 == THREAD_COUNT * MAX_COUNT);
    printf("spin3 successful!\n");
    //printf("Final spin_counter value: %d\n", spin_counter);

    return 0;
}

