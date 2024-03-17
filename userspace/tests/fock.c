#include "stdio.h"
#include "types.h"
#include "unistd.h"

int main() {
    pid_t pid = 0;

    // Create a child process
    printf("calling fork() from user space\n");
    pid = fork();

    if (pid < 0) {
        // Fork failed
        printf("Fork failed\n");
        return 1;
    } else if (pid == 0) {
        // Child process
        printf("Hello from the child process!\n");
    } else {
        // Parent process
        printf("Hello from the parent process!\n");
    }

    while (1)
    {
        /* code */
    }
    
    return 0;
}