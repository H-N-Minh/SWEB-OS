#include "pthread.h"
#include "assert.h"
#include "stdio.h"

//Test:No cancelation without cancelation point

#define BIG_NUMBER 1000000000

int function_reached = 0;
int result = 0;
int flag1 = 0;

void function()
{
    function_reached = 1;
    int counter = 0;
    while(1)
    {
        counter++;
        if(counter == BIG_NUMBER)
        {
            break;
        }
    }
    result = counter;
}

void function2(void* thread_id)
{
    while(!function_reached){}
    int rv_cancel;
    rv_cancel = pthread_cancel((size_t)thread_id);
    assert(rv_cancel == 0);
    flag1 = 1;
}


int pca3()
{
    pthread_t thread_id;
    int create_rv;
    create_rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function, NULL);
    assert(create_rv == 0);
    assert(thread_id != 0);


    pthread_t thread_id2;
    void* argument = (void*)thread_id;
    create_rv = pthread_create(&thread_id2, NULL, (void * (*)(void *))function2, argument);
    assert(create_rv == 0);
    assert(thread_id2 != 0);
    
    int join_rv2;
    void* value_ptr2;
    join_rv2 = pthread_join(thread_id2, &value_ptr2); 
    assert(join_rv2 == 0); 

    int join_rv;
    void* value_ptr;
    join_rv = pthread_join(thread_id, &value_ptr);
    assert(join_rv == 0); 

    assert(result == BIG_NUMBER);
    assert(flag1 == 1);

    printf("pca3 successful!\n");

    return 0;

}