#include "stdio.h"
#include "pthread.h"
#include "assert.h"

#define NUM_THREADS 298
#define DEBUGMINH3 0

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int count3 = 0;

int function()
{
    if (DEBUGMINH3)
    {
        pthread_mutex_lock(&mutex);
        count3++;
        printf("Thread %d waiting\n", count3);
        pthread_cond_wait(&cond, &mutex);
        printf("Thread %d woke upppppppppp\n", count3);
        pthread_mutex_unlock(&mutex);
    

        for (size_t i = 0; i < 90000000; i++)
        {
            /* code */
        }

        printf("Thread done\n");
    }
    
    int i = 4; 
    i++;
    assert(i == 5);
    return i;
    
}

//starting 250 threads
int pc3()
{
    pthread_t thread_id[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++)
    {
        int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function, NULL);
        assert(rv == 0);
    }

    if (DEBUGMINH3)
    {
        printf("-----------main Threads created done\n");
        pthread_mutex_lock(&mutex);
        pthread_cond_broadcast(&cond);
        printf("-----------main Threads broadcasted\n");
        pthread_mutex_unlock(&mutex);
    }

    for(int i = 0; i < NUM_THREADS; i++)
    {
        assert(thread_id[i] != 0);
    }
    
    printf("pc3 successful!\n");
    
    return 0;
}