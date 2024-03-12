#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void printNumber(size_t x)
{
    for (size_t i = 0; i < 2; i++)
    {
        printf("%zu\n", x);
    }
}


int main()
{
    size_t x = 3;
    size_t y = 111111111;
    pthread_t thread1;
    pthread_t thread2;

    pthread_create(&thread1, NULL, (void* (*)(void*))printNumber, (void*) x);
    pthread_create(&thread2, NULL, (void* (*)(void*))printNumber, (void*) y);

    // no pthread_join so have to loop forever to keep the process alive
    for (size_t i = 0; i < 50; i++)
    {
        printf("------------------------------------------------\n");
    }

    return 0;
}