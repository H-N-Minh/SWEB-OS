#include "pthread.h"
#include "stdio.h"
#include "assert.h"

//Test: check if basic lock and unlocking works with mutex
int mutex1()
{
    pthread_mutex_t mutex_lock;
    
    int rv;
    //mutex init
    rv = pthread_mutex_init(&mutex_lock, 0);
    assert(rv == 0);

    //locking initalized lock should work
    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv == 0);
    
    //trylocking locked lock should fail
    rv = pthread_mutex_trylock(&mutex_lock);
    assert(rv != 0);
    rv = pthread_mutex_trylock(&mutex_lock);
    assert(rv != 0);

    //locking the same lock twice should fail
    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv != 0);
    
    //unlocking a lock that is held by the current thread should work
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv == 0);

    //trylocking a unlocked thread should work and unlocking again as well
    rv = pthread_mutex_trylock(&mutex_lock);
    assert(rv == 0);
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv == 0);

    //locking a unlocked thread should work and unlocking again as well
    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv == 0);
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv == 0);

    //unlocking a not locked thread should fail
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv != 0);

    //second mutex init should fail
    rv = pthread_mutex_init(&mutex_lock, 0);
    assert(rv != 0);
    
    //using destroyed lock should fail
    rv = pthread_mutex_destroy(&mutex_lock);
    assert(rv == 0);
    rv = pthread_mutex_lock(&mutex_lock);
    assert(rv != 0);
    rv = pthread_mutex_trylock(&mutex_lock);
    assert(rv != 0);
    rv = pthread_mutex_unlock(&mutex_lock);
    assert(rv != 0);

    //destroying twice should fail
    rv = pthread_mutex_destroy(&mutex_lock);
    assert(rv != 0);

  //init after destroy should work
    rv = pthread_mutex_init(&mutex_lock, 0);
    assert(rv == 0);


    printf("mutex1 successful!\n");

    return 0;
}