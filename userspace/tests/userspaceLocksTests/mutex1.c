
#include "pthread.h"
#include "stdio.h"
#include "assert.h"

//Test: check if basic lock and unlocking works with mutex
int mutex1()
{
    pthread_mutex_t mutex_lock;
    
    int rv;
    rv = pthread_mutex_init(&mutex_lock, 0);
    assert(rv == 0);
    //printf("reached\n");
    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv == 0);
    //printf("reached\n");
    rv = pthread_mutex_trylock(&mutex_lock);
    // assert(rv != 0);
    //printf("reached\n");
    rv = pthread_mutex_trylock(&mutex_lock);
    // assert(rv != 0);
    //printf("reached\n");
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv == 0);
    //printf("reached\n");
    rv = pthread_mutex_trylock(&mutex_lock);
    // assert(rv == 0);
    //printf("reached\n");
    rv = pthread_mutex_unlock(&mutex_lock);
    // assert(rv == 0);
    //printf("reached\n");
    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv == 0);
    //printf("reached\n");
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv == 0);
    //printf("reached\n");

    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv == 0);
    //printf("reached\n");
    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv != 0);
    //printf("reached\n");"
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv == 0);
    //printf("reached");
    rv = pthread_mutex_destroy(&mutex_lock);

    printf("mutex1 successful!\n");

    return 0;
}