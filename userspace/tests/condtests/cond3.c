/*
 * This test multiple condition variables in a multi-threaded program.
 * Its good for testing "lost wake calls"
 * The main thread does the signal, children do the wait.
 */

#include "stdio.h"
#include "pthread.h"
#include <assert.h>

#define NUM_THREADS3 15
#define NUM_CONDITIONS3 14

pthread_mutex_t mymutex3[NUM_CONDITIONS3];
pthread_cond_t mycond3[NUM_CONDITIONS3];
int sharedVariable[NUM_CONDITIONS3];


void* threadFunction3(void* arg)
{
    int conditionIndex = *(int*)arg;

    pthread_mutex_lock(&mymutex3[conditionIndex]);
    while (sharedVariable[conditionIndex] != 1)
    {
        pthread_cond_wait(&mycond3[conditionIndex], &mymutex3[conditionIndex]);
    }
    pthread_mutex_unlock(&mymutex3[conditionIndex]);

    return NULL;
}


int cond3()
{
    for (int i = 0; i < NUM_CONDITIONS3; i++)
    {
        pthread_mutex_init(&mymutex3[i], NULL);
        pthread_cond_init(&mycond3[i], NULL);
        sharedVariable[i] = 0;
    }

    pthread_t threads[NUM_THREADS3];
    int conditionIndices[NUM_THREADS3];

    for (int i = 0; i < NUM_THREADS3; i++)
    {
        conditionIndices[i] = i % NUM_CONDITIONS3;
        pthread_create(&threads[i], NULL, threadFunction3, &conditionIndices[i]);
    }
    

    for (int i = 0; i < NUM_CONDITIONS3; i++)
    {
        pthread_mutex_lock(&mymutex3[i]);
        sharedVariable[i] = 1;
        pthread_cond_signal(&mycond3[i]);
        pthread_mutex_unlock(&mymutex3[i]);
    }

    for (int i = 0; i < NUM_THREADS3; i++)
    {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < NUM_CONDITIONS3; i++)
    {
        pthread_mutex_destroy(&mymutex3[i]);
        pthread_cond_destroy(&mycond3[i]);
    }

    return 0;
}