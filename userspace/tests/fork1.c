#include "stdio.h"
#include "unistd.h"
#include "assert.h"



int main()
{
    pid_t pid = fork();

    if(pid > 0)
    {
        printf("Hello from the child.\n");
    }

    else if(pid == 0)
    {
        printf("Hello from the parent.\n");
    }
    else
    {
        printf("Error\n");
    }
    

    return 0;
}