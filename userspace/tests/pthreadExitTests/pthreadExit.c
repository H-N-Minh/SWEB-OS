#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void printNumber(size_t x)
{
    // for (size_t i = 0; i < 1; i++)
    // {
    //     printf("%zu\n", x);
    // }
    assert(x == 3);
    size_t ret = 7711;
    pthread_exit((void*) ret);

}


int pe2()
{
    size_t x = 3;
    pthread_t thread1;

    int rv = pthread_create(&thread1, NULL, (void* (*)(void*))printNumber, (void*) x);
    assert(rv == 0);
    assert(thread1);

    int rv_join = pthread_join(thread1, (void**) &x);
    assert(rv_join == 0);
    assert(x == 7711);

    printf("pe2 successful!\n");

    return 0;
}