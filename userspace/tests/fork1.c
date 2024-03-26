#include "stdio.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"



int main()
{
    pid_t pid = fork();

    if(pid > 0)
    {
        printf("Hello from the parent.\n");
        pthread_t thread_id;
        
    }
    else if(pid == 0)
    {
        printf("Hello from the child.\n"); 

        pthread_t thread_id;
    }
    else
    {
        printf("Error\n");
    }

    return 0;
}
 