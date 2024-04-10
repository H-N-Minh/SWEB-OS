#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#define NUM_THREADS5 100
#define DEBUGMODE5 0

pthread_mutex_t mymutex5;

int constant = 1234;   //this value should never be changed

void* thread_function(void* arg) {
    // Acquire the mutex
    #if DEBUGMODE5 == 1
        printf("Thread %ld waiting for the mutex\n", (long)arg);
    #endif

    pthread_mutex_lock(&mymutex5);

    constant = 69;  // this should never happen

    #if DEBUGMODE5 == 1
        printf("Thread %ld acquired the mutex\n", (long)arg);
    #endif

    // Release the mutex
    pthread_mutex_unlock(&mymutex5);

    return NULL;
}

int mutex5() {
    pthread_t threads[NUM_THREADS5];

    // Initialize the mutex
    pthread_mutex_init(&mymutex5, NULL);

    pthread_mutex_lock(&mymutex5); // this forces all child thread to wait on the lock
    // Create threads
    for (long i = 0; i < NUM_THREADS5; i++) {
        pthread_create(&threads[i], NULL, thread_function, (void*)i);
    }

    assert(constant == 1234  && "some child thread got through the mutex lock and changed the constant value");
    #if DEBUGMODE5 == 1
        printf("--main thread finished pthreadcreate\n");
    #endif
    // pthread_mutex_unlock(&mymutex5); // this releases the lock and all child threads can now run
    #if DEBUGMODE5 == 1
        printf("--main thread released the lock\n");
    #endif
    // // Join threads
    // for (int i = 0; i < NUM_THREADS5; i++) {
    //     pthread_join(threads[i], NULL);
    // }

    // Destroy the mutex
    // pthread_mutex_destroy(&mymutex5);
    printf("mutex5 successful!\n");
    return 0;
}