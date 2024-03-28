#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"

int flagpc4 = 0;

int function_pca4()
{

    int oldtype;
    int rv = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);      //if this is disabled the thread should not terminate
    assert(rv == 0);
    assert(oldtype == PTHREAD_CANCEL_DEFERRED);

    // int oldstate;                                                             //if this is enabled the thread should not terminate
    // rv = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldtype);   
    // assert(rv == 0);
    // assert(oldtype == PTHREAD_CANCEL_ENABLE);
    while(1)
    {
        flagpc4 = 1;
    }
    return 4;
}

//Test: Async cancelation should cancel thread without cancelation point (Test can also be used, that is dont terminate for deffered type)
int pca4()
{
    pthread_t thread_id;

    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function_pca4, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    while(!flagpc4){}

    int rv_cancel = pthread_cancel(thread_id);
    assert(rv_cancel == 0);

    void* value_ptr;
    int rv_join = pthread_join(thread_id, &value_ptr);
    assert(rv_join == 0 && "Joining canceled thread should be successfull.");
    assert(value_ptr != 0 && "value ptr of canceled thread should be not 0.");

    printf("pca4 successfull!\n");
    return 0;
}