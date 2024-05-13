/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/


#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"


int function_pca1()
{
    while(1)
    {
        sleep(3);
    }
    return 4;
}

//Test: Cancel running thread
int main()
{
    pthread_t thread_id;

    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function_pca1, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    int rv_cancel = pthread_cancel(thread_id);
    assert(rv_cancel == 0);
    
    printf("pca1 successful!\n");
    return 0;
}