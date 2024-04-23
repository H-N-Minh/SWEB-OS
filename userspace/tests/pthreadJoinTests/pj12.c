#include "pthread.h"
#include "assert.h"
#include "stdio.h"


int functionpj12()
{
    int i = 4; 
    i++;
    return i;
}

//Test starting 2000 threads after each after with join in between
int pj12()
{

    for(int i = 0; i < 2000; i++)
    {
        pthread_t thread_id;
        int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))functionpj12, NULL);
        assert(rv == 0);
        
        void* value_ptr;
        rv = pthread_detach(thread_id);
        assert(rv == 0);
    }


    return 0;
}