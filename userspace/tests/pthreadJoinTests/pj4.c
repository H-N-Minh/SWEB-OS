#include "pthread.h"
#include "assert.h"
#include "stdio.h"

int functionpj4()
{
    sleep(1);
    return 5;
}

//Test: try to join the same thread twice
int pj4()
{
    //with value_ptr
    pthread_t thread_id1;
    int rv_create = pthread_create(&thread_id1, NULL, (void * (*)(void *))functionpj4, NULL);
    assert(rv_create == 0);
    assert(thread_id1);

    void* value_ptr;
    int rv_join = pthread_join(thread_id1, &value_ptr);
    assert(rv_join == 0);
    assert((size_t)value_ptr == 5 && "value_ptr does not match the returnvalue of thread");

    void* value_ptr1;
    value_ptr1 = (void*)42;

    int rv_join1 = pthread_join(thread_id1, &value_ptr1);
    assert(rv_join1 != 0 && "Joining the same thread twice should not be successfull");
    assert(value_ptr1 == (void*)42 && "value ptr should not get set on unsuccessfull join");
    
    printf("pj4 successful!\n");
    return 0;
}
 