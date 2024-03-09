#include "stdio.h"
#include "pthread.h"
#include <assert.h>


int function(void* simple_argument)
{
    return 0;
}


int main()
{
    pthread_t thread_id[250];

    for(int i = 0; i < 250; i++)
    {
        pthread_create(&thread_id[i], NULL, (void * (*)(void *))function, NULL);
    }

    for(int i = 0; i < 250; i++)
    {
        printf("%ld  ", thread_id[i]);
    }

    
    



    //assert(rv[3] == 0 && "Return value on succesful creation should be 0.");
    // assert(thread_id != 424242 && "thread_id was not set");
    // assert(thread_id != 0 && "UserThread has ID of KernelThread");





    return 0;
}