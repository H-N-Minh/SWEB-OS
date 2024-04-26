#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"


int functionpc7_4(char* simple_argument)
{
    assert(strcmp(simple_argument, "argument_3") == 0);
    return 31;
}

int functionpc7_3()
{
    pthread_t thread_id;
    int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpc7_4, "argument_3");
    assert(rv == 0);

    void* value_ptr; 
    rv = pthread_join(thread_id, &value_ptr);
    assert(rv == 0);
    assert((size_t)value_ptr == 31);

    return 32;
}

int functionpc7_2(char* simple_argument)
{
    assert(strcmp(simple_argument, "argument_2") == 0);

    pthread_t thread_id;
    int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpc7_3, NULL);
    assert(rv == 0);

    void* value_ptr; 
    rv = pthread_join(thread_id, &value_ptr);
    assert(rv == 0);
    assert((size_t)value_ptr == 32);

    return 33;
}

int functionpc7_1(char* simple_argument)
{
    assert(strcmp(simple_argument, "argument_1") == 0);

    pthread_t thread_id;
    int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpc7_2, "argument_2");
    assert(rv == 0);

    void* value_ptr; 
    rv = pthread_join(thread_id, &value_ptr);
    assert(rv == 0);
    assert((size_t)value_ptr == 33);

    return 34;
}


//Test: pthread create with argument
int pc7()
{
    pthread_t thread_id;
    char* simple_argument = "argument_1";

    int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpc7_1, (void*)simple_argument);
    assert(rv == 0);

    void* value_ptr; 
    rv = pthread_join(thread_id, &value_ptr);
    assert(rv == 0);
    assert((size_t)value_ptr == 34);

    return 0;
}