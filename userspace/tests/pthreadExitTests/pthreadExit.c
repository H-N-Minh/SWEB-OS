#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void printNumber(size_t x)
{
    for (size_t i = 0; i < 1; i++)
    {
        printf("%zu\n", x);
    }
    size_t ret = 7711;
    pthread_exit((void*) ret);

}


int pe2()
{
    size_t x = 3;
    pthread_t thread1;

    pthread_create(&thread1, NULL, (void* (*)(void*))printNumber, (void*) x);
    pthread_join(thread1, (void**) &x);
    
    printf("x is now %zu\n", x);
    return 0;
}