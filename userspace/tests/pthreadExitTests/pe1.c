#include "stdio.h"
#include "pthread.h"
#include "assert.h"


int function1()  //pthread_create with pthread exit in function
{
    pthread_exit((void*)3);
    return 0;
}

int pe1()
{
    int rv;

    //Test1: Simple pthread_create with pthread exit in function
    pthread_t thread_id = 424242;
    rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    assert(thread_id != 424242 && "thread_id was not set");
    assert(thread_id != 0 && "UserThread has ID of KernelThread");

    void* value_ptr;
    int pthread_join_rv = pthread_join(thread_id, &value_ptr);
    assert(value_ptr == (void*)3);

    printf("pe1 successful!\n");

    //todos join maybe


    return 0;
}