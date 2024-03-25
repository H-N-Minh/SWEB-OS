#include "stdio.h"
#include "types.h"
#include "unistd.h"

int main() {
    pid_t pid = 5;
    size_t i = 0;

    // Create a child process
    printf("calling fork() from user space\n");
    pid = fork();

    if (pid < 0) {
        // Fork failed
        printf("Fork failed\n");
        return 1;
    } else if (pid == 0) {
        // Child process
        i += 10;
        printf("Child reads: (%zu) (should be 10)..\n", i);
    } else {
        // Parent process
        i += 69;
        printf("parent reads: (%zu) (should be 69)\n", i);
    }
    // while (1)
    // {
    //     /* code */
    // }

    
    return 0;
}