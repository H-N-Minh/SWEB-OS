#include "stdio.h"
#include "pthread.h"
#include "assert.h"

#define NUM_THREADS13 600
#define ITERATIONS13 10

int function13()
{    
    int i = 4; 
    i++;
    assert(i == 5);
    return i;
}

pthread_t thread_id[NUM_THREADS13];
//starting 6000 threads in total, 600 at a time for 10 times
int pc13()
{
    for (size_t i = 0; i < ITERATIONS13; i++)
    {
        for(int i = 0; i < NUM_THREADS13; i++)
        {
            int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function13, NULL);
            assert(rv == 0);
        }

        for(int i = 0; i < NUM_THREADS13; i++)
        {
            assert(thread_id[i] != 0);
        }
        for (size_t i = 0; i < NUM_THREADS13; i++)
        {
            int rv = pthread_join(thread_id[i], NULL);
            assert(rv == 0);
        }
    }
    return 0;
}