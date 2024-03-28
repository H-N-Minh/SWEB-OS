#include "stdio.h"
#include "assert.h"
#include "pthread.h"

int flag = 0;

int function1()
{
    int delay = 0;
    for(int i = 0; i < 600000000; i++)        //TODOs replace with something nicer
    {
        delay+= i;
    }
    return 14;
}

void function2(void* thread_id)
{
    int rv = pthread_join((pthread_t)thread_id, NULL);
    assert(rv);
    assert(0 && "If this is reached the test does not test what i want it to test");
}

//Test: If a thread that called join gets canceled the joined thread should not be detached
int pj6()    //thread that called join gets canceled the joined thread should not be detached
{
    pthread_t thread_id1;
    int rv1 = pthread_create(&thread_id1, NULL, (void * (*)(void *))function1, NULL);
    assert(rv1 == 0);
    assert(thread_id1);

    void* args = (void*)thread_id1;
    pthread_t thread_id2;
    int rv2 = pthread_create(&thread_id2, NULL, (void * (*)(void *))function2, args);
    assert(rv2 == 0);
    assert(thread_id2);

    int delay = 0;
    for(int i = 0; i < 300000000; i++)        //TODOs replace with sth nicer
    {
        delay+= i;
    }

    int rv_cancel = pthread_cancel(thread_id2);
    assert(rv_cancel == 0);
    
    void* value_ptr;
    int rv_join = pthread_join(thread_id1, &value_ptr);
    assert(rv_join == -1);
    // assert((size_t)value_ptr == 14);

    int rv_join2 = pthread_join(thread_id2, &value_ptr);
    assert(rv_join2 == 0);

    printf("p2 successfull!\n");
    return 0;
}