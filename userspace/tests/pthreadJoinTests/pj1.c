#include "pthread.h"
#include "stdio.h"
#include "assert.h"

int function()
{
    return 6;
}

//pthread_join for function that has already finished 
int pj1()
{
    pthread_t thread_id;
    int pthread_create_rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function, NULL);
    assert(pthread_create_rv == 0);

    sleep(1);

    void* value_ptr;
    int pthread_join_rv = pthread_join(thread_id, &value_ptr);
    assert(pthread_join_rv == 0);
    assert((size_t)value_ptr == 6 && "value_ptr does not match the returnvalue of thread");

    return 0;
}