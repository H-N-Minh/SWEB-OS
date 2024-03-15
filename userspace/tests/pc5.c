#include "stdio.h"
#include "pthread.h"
#include <assert.h>

int function1(char* simple_argument)
{
    assert((char)simple_argument[3] == 't');
    return 34;
}

int main()
{
    pthread_t thread_id;
    char* simple_argument = "test_argument";

    int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, (void*)simple_argument);
    assert(rv == 0);

    void* value_ptr; 
    rv = pthread_join(thread_id, &value_ptr);
    assert(rv == 0);
    assert((size_t)value_ptr == 34);

    printf("pc5 successful\n");
    return 0;
}