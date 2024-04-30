#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "pthread.h"

#define PAGE_SIZE3 4096
#define THREADS3 1
#define STACK_SIZE2 (4 * 4096 - 6*sizeof(size_t)) // 4 pages - 6 bytes of metadata

pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;

size_t getTopOfThisStack3()
{
  size_t stack_variable;
  size_t top_stack = (size_t)&stack_variable - (size_t)(&stack_variable)%PAGE_SIZE3 + PAGE_SIZE3 - sizeof(size_t);
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

// Fucntion that grow to 4 pages
void* growFunc3(void* arg)
{
    printf("Thread waiting for broadcast signal before locking\n");

    // Wait for the broadcast signal from the main thread
    pthread_mutex_lock(&mutex3);
    printf("Thread waiting for broadcast signal\n");
    pthread_cond_wait(&cond3, &mutex3);
    printf("Thread received broadcast signal\n");
    pthread_mutex_unlock(&mutex3);

    printf("child Thread start growing\n");

    size_t end_stack = getTopOfThisStack() - PAGE_SIZE3 * 4 + sizeof(size_t);
    int x;
    size_t p = (size_t) &x;
    while (p >= end_stack)
    {
        *(char*) p = 'D';
        p -= 1;
    }
    
    printf("child Thread finished growing\n");
    p = end_stack;
    while (p <= (size_t) &x)
    {
        if (*(char*) p != 'D')
        {
            printf(" p is (%zu) under x and (%zu) over end_stack\n", (((size_t) &x) - p), p - end_stack);
            return (void*) -1;
        }
        p += 1;
    }

    printf("Thread finished\n");
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
    for (size_t i = 0; i < 200000000; i++)
    {
        /* code */
    }


    // Broadcast signal to all threads
    printf("Broadcasting signal before locking\n");
    pthread_mutex_lock(&mutex3);
    printf("Broadcasting signal\n");
    pthread_cond_broadcast(&cond3);
    printf("Broadcast signal sent\n");
    pthread_mutex_unlock(&mutex3);

    // Wait for all threads to finish
    int retval[THREADS3];
    for (int i = 0; i < THREADS3; i++) {
        pthread_join(threads[i], (void**) (retval + i));
    }
    for (size_t i = 0; i < THREADS3; i++)
    {
        if (retval[i] != 69)
        {
            printf("Thread %zu failed\n", i);
            return -1;
        }
    }
    return 0;
}
