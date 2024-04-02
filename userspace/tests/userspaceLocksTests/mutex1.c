
#include "pthread.h"
#include "stdio.h"
#include "assert.h"

//Test: check if basic lock and unlocking works with mutex
int mutex1()
{
    pthread_mutex_t lock;
    
    int rv;
    rv = pthread_mutex_init(&lock, 0);
    assert(rv == 0);
    rv = pthread_mutex_lock(&lock);
    assert(rv == 0);
    rv = pthread_mutex_trylock(&lock);
    assert(rv != 0);
    rv = pthread_mutex_trylock(&lock);
    assert(rv != 0);
    rv = pthread_mutex_unlock(&lock);
    assert(rv == 0);
    rv = pthread_mutex_trylock(&lock);
    assert(rv == 0);
    rv = pthread_mutex_unlock(&lock);
    assert(rv == 0);
    rv = pthread_mutex_lock(&lock);
    assert(rv == 0);
    rv = pthread_mutex_unlock(&lock);
    assert(rv == 0);

    printf("mutex1 successful!\n");

    return 0;
}