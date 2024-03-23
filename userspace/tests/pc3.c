#include "stdio.h"
#include "pthread.h"
#include "assert.h"


int function()
{
    int i = 4; 
    i++;
    assert(i == 5);
    return i;
}

//Test starting 
int main()
{
    pthread_t thread_id[250];

    for(int i = 0; i < 250; i++)
    {
        int rv = pthread_create(&thread_id[i], NULL, (void * (*)(void *))function, NULL);
        assert(rv == 0);
    }

    for(int i = 0; i < 250; i++)
    {
        assert(thread_id[i] != 0);
    }
    
    printf("pc3 successful!\n");
    
    return 0;
}