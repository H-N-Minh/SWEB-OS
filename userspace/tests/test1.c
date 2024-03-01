#include "stdio.h"
#include "pthread.h"

void function()
{
    printf("Function called!");
}

int main()
{
    pthread_t thread_id = 0;
    printf("pthread_creates returns %d und has thread_id %ld \n", pthread_create(&thread_id, NULL, (void * (*)(void *))function, NULL), thread_id);
    return 0;
}

