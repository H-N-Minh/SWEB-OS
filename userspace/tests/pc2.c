#include "stdio.h"
#include "pthread.h"
#include "assert.h"


int function1()
{
    return 0;
}

int main()
{
    int rv;

    //Test1: Simple pthread_create and check if thread id gets set
    pthread_t thread_id = 424242;
    rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
    assert(rv == 0 && "Return value on succesful creation should be 0.");
    assert(thread_id != 424242 && "thread_id was not set");
    assert(thread_id != 0 && "UserThread has ID of KernelThread");

    printf("pc2 successful!\n");

    return 0;
}