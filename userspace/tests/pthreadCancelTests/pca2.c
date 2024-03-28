#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"


int function_pca2()
{
    return 4;
}

//Test: Cancel running function
int pca2()
{
    pthread_t thread_id;

    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function_pca2, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    int delay = 0;
    for(int i = 0; i < 300000000; i++)        //TODOs replace with sleep
    {
        delay+= i;
    }

    int rv_cancel = pthread_cancel(thread_id);
    assert(rv_cancel == -1 && "Thread is alread dead and cannot be canceled");
    
    printf("pca2 successfull!\n");
    return 0;
}