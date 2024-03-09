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

    for(int i = 0; i < 300; i++)        //not thread safe version to let function terminate before main -> maybe sleep would be nicer
    {
        printf(".");
    }

    void* value_ptr;
    int pthread_join_rv = pthread_join(thread_id, &value_ptr);
    assert(pthread_join_rv == 0);

    assert((size_t)value_ptr == 6 && "value_ptr does not match the returnvalue of thread");

    printf("pc5 successful!\n");
}