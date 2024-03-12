#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void printNumber(size_t x)
{
    for (size_t i = 0; i < 1; i++)
    {
        printf("%zu\n", x);
    }
    size_t ret = 69;
    pthread_exit((void*) ret);
}


int main()
{
    size_t x = 3;
    pthread_t thread1;

    pthread_create(&thread1, NULL, (void* (*)(void*))printNumber, (void*) x);
    while (1)
    {
        /* code */
    }
    
    printf("main thread done\n");
    return 0;
}