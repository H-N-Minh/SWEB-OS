
#include "pthread.h"
#include "stdio.h"
#include "assert.h"

//Test: check if basic lock and unlocking works with spinnlock
int spin1()
{
    pthread_spinlock_t lock;
    
    int rv;
    //spin init
    rv = pthread_spin_init(&lock, 0);
    assert(rv == 0);

    //locking initalized lock should work
    rv = pthread_spin_lock(&lock);
    assert(rv == 0);
    
    //trylocking locked lock should fail
    rv = pthread_spin_trylock(&lock);
    assert(rv != 0);
    rv = pthread_spin_trylock(&lock);
    assert(rv != 0);

    //locking the same lock twice should fail
    rv = pthread_spin_lock(&lock);
    assert(rv != 0);
    
    //unlocking a lock that is held by the current thread should work
    rv = pthread_spin_unlock(&lock);
    assert(rv == 0);

    //trylocking a unlocked thread should work and unlocking again as well
    rv = pthread_spin_trylock(&lock);
    assert(rv == 0);
    rv = pthread_spin_unlock(&lock);
    assert(rv == 0);

    //locking a unlocked thread should work and unlocking again as well
    rv = pthread_spin_lock(&lock);
    assert(rv == 0);
    rv = pthread_spin_unlock(&lock);
    assert(rv == 0);

    //unlocking a not locked thread should fail
    rv = pthread_spin_unlock(&lock);
    assert(rv != 0);

    //second spin init should fail
    rv = pthread_spin_init(&lock, 0);
    assert(rv != 0);
    
    //using destroyed lock should fail
    rv = pthread_spin_destroy(&lock);
    assert(rv == 0);
    rv = pthread_spin_lock(&lock);
    assert(rv != 0);
    rv = pthread_spin_trylock(&lock);
    assert(rv != 0);
    rv = pthread_spin_unlock(&lock);
    assert(rv != 0);

    //destroying twice should fail
    rv = pthread_spin_destroy(&lock);
    assert(rv != 0);

  //init after destroy should work
    rv = pthread_spin_init(&lock, 0);
    assert(rv == 0);

    printf("spin1 successful!\n");

    return 0;
}