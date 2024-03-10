#include "stdio.h"
#include "pthread.h"
#include <assert.h>


int function1(void* simple_argument)
{
    // int array[100];
    // for(int i = 0; i < 100; i++)
    // {
    //     array[i] = i;
    // }
    // for(int i = 0; i < 100; i++)
    // {
    //     printf("%d ", array[i]);
    // }
    return 0;
}

int function2(void* simple_argument)
{
    while(1){}
    return 0;
}

int main()
{
    int rv;

    // Test2: 2 Threads
    pthread_t thread_id1 = 424242;
    pthread_t thread_id2 = 424242;
    rv = pthread_create(&thread_id1, NULL, (void * (*)(void *))function2, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    rv = pthread_create(&thread_id2, NULL, (void * (*)(void *))function2, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    assert(thread_id1 != 424242 && "thread_id was not set");
    assert(thread_id2 != 424242 && "thread_id was not set");
    assert(thread_id1 != thread_id2 && "Two running thread cant have the same id.");

    printf("pc6 successful!\n");


    return 0;
}