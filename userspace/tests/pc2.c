#include "stdio.h"
#include "pthread.h"
#include <assert.h>


int function1(void* simple_argument)
{
    return 0;
}

int function2(void* simple_argument)
{
    while(1){}
    return 0;
}

int main()
{
    int rv;

    //Test1: Simple pthread_create
    pthread_t thread_id = 424242;
    rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    assert(thread_id != 424242 && "thread_id was not set");
    assert(thread_id != 0 && "UserThread has ID of KernelThread");



    //Test2: Simple pthread_create
    pthread_t thread_id1 = 424242;
    pthread_t thread_id2 = 424242;
    rv = pthread_create(&thread_id1, NULL, (void * (*)(void *))function2, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    rv = pthread_create(&thread_id2, NULL, (void * (*)(void *))function2, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    assert(thread_id1 != 424242 && "thread_id was not set");
    assert(thread_id2 != 424242 && "thread_id was not set");
    assert(thread_id1 != thread_id2 && "Two running thread cant have the same id.");
    
    






    printf("pc2 successful!\n");

    //TODO: nonesene attribute

    return 0;
}