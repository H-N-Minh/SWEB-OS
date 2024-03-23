
#include "pthread.h"
#include "stdio.h"
#include "assert.h"

int main()
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





    printf("l1 successful\n");
}