#include "stdio.h"
#include "pthread.h"
#include <assert.h>


int function()
{
    int i = 4; 
    i++;
    return i;
}

//Test starting 2000 threads after each after with join in between
int main()
{

    for(int i = 0; i < 2000; i++)
    {
        pthread_t thread_id;
        int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function, NULL);
        assert(rv == 0);
        
        void* value_ptr;
        rv = pthread_join(thread_id, &value_ptr);
        assert(rv == 0);
        assert((size_t)value_ptr == 5);
    }

    printf("pj3 successful!\n");
    
    return 0;
}