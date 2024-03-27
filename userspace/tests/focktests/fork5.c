#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

#define MAX_FORK 50     // without swaping, can fork around 150 times max

// this tests multiple nested forks. each of them should has their own version of the variable x
int fork5() {  
    size_t x = 0;
    // create 100 child processes
    for (int i = 0; i < MAX_FORK; i++) {
        assert(x == i  && "each process should have its own unique value of x");
        pid_t pid = fork();
        if (pid > 0) {
            x += 1;      
            continue;    // parent continues to fork
        } 
        else if (pid == 0) {   // child dies
            assert(x == i  && "value x of the child should be the same as parent after fork");
            return CHILD_SUCCESS;       // it says child process but the name here is only relative
        } 
        else {
            // handle fork error
            return -1;
        }
    }

    // only 1 last process would reach here
    assert(x == MAX_FORK  && "last process (the 100th process) should have x = 100");
    return PARENT_SUCCESS;
}