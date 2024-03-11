#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void printNumber(size_t x)
{
    printf("From 'printNumber' func, para is %zu\n", x);
}


int main()
{
    size_t x = 69;

    printf("From 'main' func, before pthread_create\n");
    pthread_create(NULL, NULL, (void* (*)(void*))printNumber, (void*) x);
    printf("From 'main' func, after pthread_create\n");

    assert(0);      // Why is this never reached?
    assert(1);
    return 0;
}