#include "pthread.h"
#include <stdio.h>
#include <assert.h>

int function()
{
    return 6;
}


int main()
{
    pthread_t thread_id;
    int pthread_create_rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function, NULL);
    assert(pthread_create_rv == 0);

    int delay = 0;
    for(int i = 0; i < 300000000; i++)        //replace with sleep
    {
        delay+= i;
    }

    void* value_ptr;
    int pthread_join_rv = pthread_join(thread_id, &value_ptr);
    assert(pthread_join_rv == 0);
    assert((size_t)value_ptr == 6 && "value_ptr does not match the returnvalue of thread");

    printf("pj1 successful!\n");
}