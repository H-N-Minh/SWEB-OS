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


int function1pc2()
{
    return 0;
}

int main()
{
    int rv;

    //Test1: Simple pthread_create and check if thread id gets set
    pthread_t thread_id = 424242;
    rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1pc2, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    assert(thread_id != 424242 && "thread_id was not set");
    assert(thread_id != 0 && "UserThread has ID of KernelThread");
    pthread_join(thread_id, NULL);

    return 0;
}