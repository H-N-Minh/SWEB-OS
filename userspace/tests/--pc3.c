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

#define NUM_THREADS 250

int count3 = 0;

int function()
{    
    int i = 4; 
    i++;
    assert(i == 5);
    return i;
}

//starting 250 threads
int main()
{
    pthread_t thread_id[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++)
    {
        int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function, NULL);
        assert(rv == 0);
    }

    for(int i = 0; i < NUM_THREADS; i++)
    {
        assert(thread_id[i] != 0);
    }
    for (size_t i = 0; i < NUM_THREADS; i++)
    {
        int rv = pthread_join(thread_id[i], NULL);
        assert(rv == 0);
    }
    
    
    return 0;
}