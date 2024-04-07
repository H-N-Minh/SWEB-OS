#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success


int fork3_global_var = 0;

int function1()
{
    fork3_global_var += 369;
    return 0;
}

// this tests fork together with pthread_create
int fork3() {    
    pid_t pid = fork();

    if (pid < 0) 
    {
        printf("Error\n");
    }
    else if (pid == 0) // child
    {
        pthread_t thread_id;

        int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
        assert(rv == 0 && "child process failed to use pthread_create");

        pthread_join(thread_id, NULL);
        assert(fork3_global_var == 369 && "pthread_create of child process should have changed fork3_global_var to 369");

        return CHILD_SUCCESS;

    } else {    // parent
        pthread_t thread_id;

        fork3_global_var += 111;    // so parent and child has different values of fork3_global_var before pthread_create
        int rv = pthread_create(&thread_id, NULL, (void * (*)(void *))function1, NULL);
        assert(rv == 0 && "child process failed to use pthread_create");

        pthread_join(thread_id, NULL);
        assert(fork3_global_var == 480 && "pthread_create of child process should have changed fork3_global_var to 480");

        return PARENT_SUCCESS;
    }
    return 0;
}