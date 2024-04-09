#include "pthread.h"
#include "assert.h"

int sc2()
{
    int oldstate;
    int oldtype;
    int return_value;

    return_value = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    assert(oldstate == PTHREAD_CANCEL_ENABLE);
    assert(return_value == 0);

    return_value = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
    assert(oldtype == PTHREAD_CANCEL_DEFERRED);
    assert(return_value == 0);

    //round 2
    return_value = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    assert(oldstate == PTHREAD_CANCEL_DISABLE);
    assert(return_value == 0);

    return_value = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
    assert(oldtype == PTHREAD_CANCEL_ASYNCHRONOUS);
    assert(return_value == 0);

    //round 3
    return_value = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    assert(oldstate == PTHREAD_CANCEL_DISABLE);
    assert(return_value == 0);

    return_value = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    assert(oldtype == PTHREAD_CANCEL_ASYNCHRONOUS);
    assert(return_value == 0);

    printf("sc2 successful");

    return 0;
}
