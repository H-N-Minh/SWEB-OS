#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void function1(size_t x)
{
    return 0;
}

int main()
{
    size_t x = 69;
    pthread_create(NULL, NULL, (void* (*)(void*))function1, (void*) x);

    return 0;
}