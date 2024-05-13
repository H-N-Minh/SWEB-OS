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


int function_pca2()
{
    return 4;
}

//Test: Cancel dead thread
int main()
{
    pthread_t thread_id;

    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function_pca2, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    sleep(1);

    int rv_cancel = pthread_cancel(thread_id);
    assert(rv_cancel == -1 && "Thread is alread dead and cannot be canceled");
    
    printf("pca2 successful!\n");
    return 0;
}