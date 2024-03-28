#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"

int function1()
{
    printf("Hello from child threads friend\n");
    while (1)
    {
        /* code */
    }
    
    return 0;
}



int fork2() {
    pid_t pid = 5;
    size_t i = 0;


    //Test1: Simple pthread_create and check if thread id gets set

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


        pthread_t thread_id = 424242;
        pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);


    } else {
        // Parent process
        i += 69;
        printf("parent reads: (%zu) (should be 69)\n", i);
    }
    // while (1)
    // {
    //     /* code */
    // }

    
    return pid;
}