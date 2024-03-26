#include "stdio.h"
#include "types.h"
#include "unistd.h"

int main() {
    size_t i = 0;

    // Create a child process
    printf("calling fork() from user space\n");
    pid_t pid = fork();

    if (pid < 0) 
    {
        printf("Error\n");
    }
    else if (pid == 0) 
    {
        // Child process
        i += 10;
        printf("Child reads: (%zu) (should be 10)..\n", pid);
    } else {
        // Parent process
        i += 69;
        printf("parent reads: (%zu) (should be 69)..\n", pid);
    }
    // while (1)
    // {
    //     /* code */
    // }

    
    return 0;
}