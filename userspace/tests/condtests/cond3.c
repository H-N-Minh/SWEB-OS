#include "stdio.h"
#include "pthread.h"
#include <assert.h>

#define NUM_THREADS3 5

pthread_mutex_t mymutex3;           
pthread_cond_t mycond3;             // main thread use this to signal child thread
pthread_cond_t second_mycond3;    // when child thread finish, it signals the main thread using this
int flag3 = 0;

void* threadFunction3(void* arg)
{
    pthread_mutex_lock(&mymutex3);
    while (flag3 == 0)
    {
        pthread_cond_wait(&mycond3, &mymutex3);
    }
    // 2. this happens second
    flag3--;
    pthread_cond_signal(&second_mycond3);
    pthread_mutex_unlock(&mymutex3);

    return NULL;
}

int cond3() {
    pthread_t threads[NUM_THREADS3];
    pthread_mutex_init(&mymutex3, NULL);
    pthread_cond_init(&mycond3, NULL);
    pthread_cond_init(&second_mycond3, NULL);

    for (int i = 0; i < NUM_THREADS3; i++)
    {
        pthread_create(&threads[i], NULL, threadFunction3, NULL);
    }

    pthread_mutex_lock(&mymutex3);
    for (int i = 0; i < NUM_THREADS3; i++)
    {
        // 1. this happens first
        flag3++;
        pthread_cond_signal(&mycond3);
        
        while (flag3 == 1)
        {
            pthread_cond_wait(&second_mycond3, &mymutex3);
        }
    }
    pthread_mutex_unlock(&mymutex3);


    for (int i = 0; i < NUM_THREADS3; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mymutex3);
    pthread_cond_destroy(&mycond3);

    return 0;
}