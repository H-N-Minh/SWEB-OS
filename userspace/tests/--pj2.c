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


int function1pj2()
{
    sleep(1);
    return 5;
}

//Test: pthread join where join has to wait for the function to finish
int main()
{
    //without value_ptr
    pthread_t thread_id;
    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function1pj2, NULL);
    assert(rv_create == 0);
    assert(thread_id);
    int rv_join = pthread_join(thread_id, NULL);
    assert(rv_join == 0);

    //with value_ptr
    pthread_t thread_id1;
    rv_create = pthread_create(&thread_id1, NULL, (void * (*)(void *))function1pj2, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    void* value_ptr;
    rv_join = pthread_join(thread_id1, &value_ptr);
    assert(rv_join == 0);
    assert((size_t)value_ptr == 5 && "value_ptr does not match the returnvalue of thread");
    

    return 0;
}