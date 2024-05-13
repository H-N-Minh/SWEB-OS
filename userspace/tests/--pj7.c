/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/

#include "pthread.h"
#include "stdio.h"
#include "assert.h"

int function_pj7_reached = 0;

void function_pj7()
{
    function_pj7_reached = 1;
}

//join void function
int main()
{
    pthread_t thread_id;
    int pthread_create_rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function_pj7, NULL);
    assert(pthread_create_rv == 0);

    while(!function_pj7_reached){}

    int pthread_join_rv = pthread_join(thread_id, NULL);
    assert(pthread_join_rv == 0);


    return 0;
}