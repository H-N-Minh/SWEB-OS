#include "stdio.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"



int fork1()
{
    pid_t pid = fork();

    if(pid > 0)
    {
        printf("Hello from the parent.\n");
    }
    else if(pid == 0)
    {
        printf("Hello from the child.\n"); 
    }
    else
    {
        printf("Error\n");
    }

    return pid;
}
 