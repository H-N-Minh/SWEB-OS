#include "stdio.h"
#include "pthread.h"

void function()
{
    printf("Function called!");
    while(1)
    {
        //printf("hey\n");
    }
}

int main()
{
    pthread_t thread_id;
    printf("pthread_creates returns %d \n", pthread_create(&thread_id, NULL, (void * (*)(void *))function, NULL));
    while(1)
    {
        //printf("hey\n");
    }
    return 0;
}

