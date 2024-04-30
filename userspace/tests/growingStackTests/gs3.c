#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "pthread.h"

#define PAGE_SIZE3 4096
#define THREADS3 1
#define STACK_SIZE2 (4 * 4096 - 6) // 4 pages - 6 bytes of metadata

pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;

// Fucntion that grow to 4 pages
void* growFunc3(void* arg)
{
    printf("Thread waiting for broadcast signal before locking\n");
    return (void*) 69;
    // Wait for the broadcast signal from the main thread
    pthread_mutex_lock(&mutex3);
    printf("Thread waiting for broadcast signal\n");
    pthread_cond_wait(&cond3, &mutex3);
    printf("Thread received broadcast signal\n");
    pthread_mutex_unlock(&mutex3);

    char stack_data[STACK_SIZE2];
    for (int i = 0; i < STACK_SIZE2; i++)
    {
        stack_data[i] = 'A';
    }

    
    for (size_t i = (STACK_SIZE2 - 1); i >= 0; i--)
    {
        if (stack_data[i] != 'A')
        {
        return (void*) -1;
        }
    }

    return (void*) 69;
}

// Test a very basic growing stack
int gs3()
{
    pthread_t threads[THREADS3];

    // Create threads and wait for broadcast signal
    printf("Creating threads with func at %p\n", growFunc3);
    for (int i = 0; i < THREADS3; i++) {
        pthread_create(&threads[i], NULL, growFunc3, NULL);
    }
    printf("Threads created\n");
    // // some delay so all threads can be initialized
    // for (size_t i = 0; i < 200000000; i++)
    // {
    //     /* code */
    // }

    int retval[THREADS3];
    for (int i = 0; i < THREADS3; i++) {
        pthread_join(threads[i], (void**) (retval + i));
    }
    printf("Threads joined\n");
    

    // Broadcast signal to all threads
    // printf("Broadcasting signal before locking\n");
    // pthread_mutex_lock(&mutex3);
    // printf("Broadcasting signal\n");
    // pthread_cond_broadcast(&cond3);
    // printf("Broadcast signal sent\n");
    // pthread_mutex_unlock(&mutex3);

    // Wait for all threads to finish
    // int retval[THREADS3];
    // for (int i = 0; i < THREADS3; i++) {
    //     pthread_join(threads[i], (void**) (retval + i));
    // }
    // for (size_t i = 0; i < THREADS3; i++)
    // {
    //     if (retval[i] != 69)
    //     {
    //         printf("Thread %zu failed\n", i);
    //         return -1;
    //     }
    // }
    return 0;
}
