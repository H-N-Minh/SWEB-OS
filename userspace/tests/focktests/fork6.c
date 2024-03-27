#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

#define NUM_THREADS 20      // without swaping, max is 20 threads, which fork into 40 threads and 20 processes

int fork6_global_var = 0;

// test the locking of archmemory: create multiple threads of same process then fork at same time,
// NOTE: not a really good test for locking, more like testing pthread_create and fork at the same time
void* threadFunction(void* arg) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        fork6_global_var += 69;
        assert(fork6_global_var == 69 && "fork6_global_var should be 69 in every child process");
    } else if (pid > 0) {
        // Parent process
        assert(fork6_global_var == 0 && "fork6_global_var should be 0 in every threads of parent process");
    } else {
        // Fork failed
        printf("Fork failed in fork6\n");
    }
    
    return NULL;
}

int fork6() {
    pthread_t threads[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, threadFunction, NULL) != 0) {
            return -1;
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return PARENT_SUCCESS;      // if reach this point without assert error then child process is successful too
}