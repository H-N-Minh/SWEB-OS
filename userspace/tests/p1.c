#include "stdio.h"
#include "pthread.h"

int function1(void* simple_argument)
{
    printf("\nFunction 1 called!\n");
    printf("Function 1 received argument %s\n",(char*)simple_argument);
    return 34;
}

int main()
{
    pthread_t thread_id;
    char* simple_argument = "test_argument";

    printf("pthread_creates returns %d \n", pthread_create(&thread_id, NULL, (void * (*)(void *))function1, (void*)simple_argument));
    //printf("pthread_creates returns %d \n", pthread_create(&thread_id, NULL, (void * (*)(void *))function, NULL));
    //while(1){}
    // for(int i; i < 400; i++)
    // {
    //     printf(".");
    // }
    return 0;
}