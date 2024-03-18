#include "stdio.h"
#include "pthread.h"

#define NUM_THREADS 100
#define MAX_COUNT   1000

int counter = 0;

void increment()
{
    for (int i = 0; i < 1000000; i++)
    {
        counter++;
    }
    printf("Counter value: %d\n", counter);
}

int main() {
    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, (void* (*)(void*))increment, NULL);
    pthread_create(&thread2, NULL, (void* (*)(void*))increment, NULL);

    printf("Counter value: %d\n", counter);

    while(1)
    {

    }

    return 0;
}
