#include "stdio.h"
#include "unistd.h"
#include "assert.h"

int main()
{
    size_t i = 0;

    // Create a child process
    printf("calling fork() from user space\n");
    pid_t pid = fork();

    if (pid > 0)
    {
        printf("Hello from the parenttt69 with child id (%zu).\n", pid);
    }
    else if (pid == 0)
    {
        printf("Hello from the childd69.\n");
    }
    else
    {
        printf("Error\n");
    }
    // while (1)
    // {
    //     /* code */
    // }
    

    return 0;
}