#include "stdio.h"
#include "pthread.h"

#define NUM_THREADS 100
#define MAX_COUNT   1000

int counter1 = 0;

void increment()
{
    for (int i = 0; i < 1000000; i++)
    {
        counter1++;
    }
    printf("Counter value: %d\n", counter1);
}

int main() {
    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, (void* (*)(void*))increment, NULL);
    pthread_create(&thread2, NULL, (void* (*)(void*))increment, NULL);

    printf("Counter value: %d\n", counter1);

    while(1)
    {

    }

    return 0;
}
