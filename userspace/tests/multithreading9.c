#include "stdio.h"
#include "pthread.h"
#include <assert.h>


int function1()
{
    return 4;
}

//Test: pthread join where function has already be terminated when pthread_join is reached
int main()
{
    pthread_t thread_id;


    int rv_create = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
    assert(rv_create == 0);
    assert(thread_id);

    printf("\n");
    for(int i = 1; i < 1000; i++)
    {
        printf("%d ",i);
    }
    printf("\n\n\n");
    
    void* rv_of_function;
    int rv_join = pthread_join(thread_id, &rv_of_function);
    
    assert(rv_join == 0);
    assert((size_t)rv_of_function == 4);
    
    
    printf("Multithreading 9 successful!\n");



    return 0;
}
