#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"


int function1()
{
    while(1)
    {
        sleep(3);
    }
    return 4;
}

//Test: Cancel running function
int main()
{
    pthread_t thread_id;

    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    int rv_cancel = pthread_cancel(thread_id);
    assert(rv_cancel == 0);
    
    printf("pca1 successfull!\n");
    return 0;
}