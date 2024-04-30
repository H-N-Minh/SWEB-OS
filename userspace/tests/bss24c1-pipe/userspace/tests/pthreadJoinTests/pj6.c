#include "stdio.h"
#include "assert.h"
#include "pthread.h"

int join_rv = 10;

int function1()
{
    sleep(3);
    return 14;
}

int function2(void* thread_id)
{
    void* value_ptr;
    join_rv = pthread_join((pthread_t)thread_id, &value_ptr);
    assert(join_rv == 0);
    assert(value_ptr == (void*)14);
    for(int i = 0; i < 100; i++)
    {
        join_rv++;
    }
    return 6;
}

//more complex join and cancel test
int pj6()
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

    sleep(1);

    int rv_cancel = pthread_cancel(thread_id2);
    assert(rv_cancel == 0);
    
    void* value_ptr;
    int rv_join = pthread_join(thread_id1, &value_ptr);
    assert(rv_join != 0);

    void* value_ptr1;
    int rv_join2 = pthread_join(thread_id2, &value_ptr1);
    assert(rv_join2 == 0);
    assert(value_ptr1 != 0);

    assert(join_rv == 100); //global value



    return 0;
}