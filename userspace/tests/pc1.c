#include "stdio.h"
#include "pthread.h"
#include <assert.h>


int function1(void* simple_argument)
{
    return 0;
}

int main()
{
    pthread_t thread_id;
    int rv;


    //Test1: No ptr to thread_id provided
    rv = pthread_create(NULL, NULL, (void * (*)(void *))function1, NULL);
    assert(rv != 0 && "Test1: No ptr to thread_id provided");

    //Test2: No ptr to start_routine provided
    rv = pthread_create(&thread_id, NULL, NULL, NULL);
    assert(rv != 0 && "Test2: No ptr to start_routine provided");

    //Test3: Invalid adress provided as pthread id
    rv = pthread_create((void*)0xffff80ff00000000ULL, NULL, (void * (*)(void *))function1, NULL);
    assert(rv != 0 && "Test3: Invalid adress provided as pthread id");

    //Test4: Invalid ptr to start_routine provided
    rv = pthread_create(&thread_id, NULL, (void * (*)(void *))0x0400800000000000ULL, NULL);
    assert(rv != 0 && "Test4: Invalid ptr to start_routine provided");

    //Test5: Invalid ptr to args provided
    rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, (void*)0x0000900000000000ULL);
    assert(rv != 0 && "Test5: Invalid ptr to args provided");

    //Test6: Invalid ptr to attr provided
    rv = pthread_create(&thread_id, (void*)0x0000800000000000ULL, (void * (*)(void *))function1, NULL);
    assert(rv != 0 && "Test6: Invalid ptr to attr provided");

    printf("pc1 successful!\n");

    return 0;
}