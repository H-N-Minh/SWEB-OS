#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "pthread.h"

#define PAGE_SIZE3 4096
#define THREADS3 1
#define STACK_SIZE2 (4 * 4096 - 6) // 4 pages - 6 bytes of metadata

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

    size_t end_stack = getTopOfThisStack() - PAGE_SIZE3 * 4 + 1;
    int x = 0;
    size_t p = (size_t) &x;
    printf("-------------last position is %zu\n", p - STACK_SIZE2 + 1);
    for (int i = (STACK_SIZE2 - 1); i >= 0; i--)
    {
        printf("i: %d, at (%p) [%zu]\n", i, (int*) (p - i), p - i);
        if (i < 12430)
        {
            printf("i: %d, at (%p) [%zu]\n", i, (int*) (p - i), p - i);
            /* code */
        }
        
        *(int*) (p - i) = 'D';
    }
    printf("child Thread finished growing\n");
    for (int i = 0; i < STACK_SIZE2; i++)
    {
        if (*(int*) (p - i) != 'D')
        {
        return (void*) -1;
        }
    }
    printf("Thread finished\n");
    return (void*) 69;
}

// Test a very basic growing stack
int main()
{
    pthread_t threads[THREADS3];

    printf("Creating threads with func at %p\n", growFunc3);
    for (int i = 0; i < THREADS3; i++) {
        pthread_create(&threads[i], NULL, growFunc3, NULL);
    }
    printf("main: Threads created\n");

    for (size_t i = 0; i < 200000000; i++)
    {
        /* code */
    }

    // Broadcast signal to all threads
    printf("main locking\n");
    pthread_mutex_lock(&mutex3);
    printf("main Broadcasting signal\n");
    pthread_cond_broadcast(&cond3);
    printf("main Broadcast signal sent\n");
    pthread_mutex_unlock(&mutex3);

    // Wait for all threads to finish
    int retval[THREADS3];
    for (int i = 0; i < THREADS3; i++) {
        printf("main: Joining thread %d\n", i);
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
    printf("main: Threads joined\n");
    
    return 0;
}
