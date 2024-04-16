#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

// Test a very basic fork
int fork1()
{
    pid_t pid = fork();

    if (pid == -1) {
        return -1;
    } else if (pid == 0) {
        //printf("Hello from child process!\n");
        return CHILD_SUCCESS;
    } else {
        //printf("Hello from parent process!\n");
    }

    return PARENT_SUCCESS;
}