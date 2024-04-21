#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"

int functionpc5(char* simple_argument)
{
    assert(strcmp(simple_argument, "test_argument") == 0);
    return 34;
}



//Test: pthread create with argument
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

    return 0;
}