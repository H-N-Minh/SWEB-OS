#include "stdio.h"
#include "pthread.h"
#include <assert.h>


int function1()
{
    for(int i = 1; i < 1000; i++)
    {
        printf("%d ",i);
    }
    return 4;
}

int main()
{
    pthread_t thread_id;


    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    int rv_join = pthread_join(thread_id, NULL);
    assert(rv_join == 0);
    
    
    printf("Multithreading 8 successful!\n");


    return 0;
}