#include "pthread.h"
#include "assert.h"

int main()
{
    int oldstate;
    int oldtype;
    int rv;
    rv = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    assert(oldstate == PTHREAD_CANCEL_ENABLE);
    assert(rv == 0);
    rv = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    assert(oldstate == PTHREAD_CANCEL_DISABLE);
    assert(rv == 0);
    rv = pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, &oldstate);
    assert(rv == -1);

    rv = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
    assert(oldtype == PTHREAD_CANCEL_DEFERRED);
    assert(rv == 0);
    rv = pthread_setcancelstate(345, &oldtype);
    assert(rv == -1);
    rv = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    assert(oldtype == PTHREAD_CANCEL_ASYNCHRONOUS);
    assert(rv == 0);

    printf("sc1 successfull");
}

#