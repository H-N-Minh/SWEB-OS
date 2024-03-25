#include "stdio.h"
#include "unistd.h"
#include "assert.h"



int main()
{
    pid_t pid = fork();

    if(pid > 0)
    {
        printf("Hello from the parent.\n");
        while(1){}
    }
    else if(pid == 0)
    {
        printf("Hello from the child.\n"); 
        while(1){}
    }
    else
    {
        printf("Error\n");
    }
    return 0;
}
 