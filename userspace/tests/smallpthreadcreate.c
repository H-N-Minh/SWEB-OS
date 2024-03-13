#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void testFunc(size_t x)
{
    printf("thread created with %zu", x);
}

int main()
{
    size_t x = 100;
    printf("called func?\n");
    printf("------------------ %p", testFunc);
    pthread_create(NULL, NULL, (void* (*)(void*))testFunc, (void*) x);
    printf("func is called\n");

    return 0;
}
