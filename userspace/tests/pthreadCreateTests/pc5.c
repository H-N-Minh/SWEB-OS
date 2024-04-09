#include "stdio.h"
#include "pthread.h"
#include "assert.h"

int functionpc5(char* simple_argument)
{
    assert((char)simple_argument[3] == 't');          //TODOs make check nicer
    return 34;
}


//Test: pthread create with simple argument
int pc5()
{
    pthread_t thread_id;
    char* simple_argument = "test_argument";

    int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpc5, (void*)simple_argument);
    assert(rv == 0);

    void* value_ptr; 
    rv = pthread_join(thread_id, &value_ptr);
    assert(rv == 0);
    assert((size_t)value_ptr == 34);

    printf("pc5 successful!\n");
    return 0;
}