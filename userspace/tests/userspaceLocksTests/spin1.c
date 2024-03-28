
#include "pthread.h"
#include "stdio.h"
#include "assert.h"

//Test: check if basic lock and unlocking works with spinnlock
int spin1()
{
    pthread_spinlock_t lock;
    
    int rv;
    rv = pthread_spin_init(&lock, 0);
    assert(rv ==0);
    rv = pthread_spin_lock(&lock);
    assert(rv == 0);
    rv = pthread_spin_trylock(&lock);
    assert(rv != 0);
    rv = pthread_spin_trylock(&lock);
    assert(rv != 0);
    rv = pthread_spin_unlock(&lock);
    assert(rv == 0);
    rv = pthread_spin_trylock(&lock);
    assert(rv == 0);
    rv = pthread_spin_unlock(&lock);
    assert(rv == 0);
    rv = pthread_spin_lock(&lock);
    assert(rv == 0);
    rv = pthread_spin_unlock(&lock);
    assert(rv == 0);

    printf("spin1 successful!\n");

    return 0;
}