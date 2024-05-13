/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0]
disabled: false
*/


/**
 * This test the "lost wake call", which is when a thread is holding a lock, about to go to sleep so another thread
 * can take that lock and wake first thread up
 * Optimal is thread1 release the lock and go to sleep, thread2 take the lock and wake up thread1
 * problem is when thread1 release the lock, thread 2 take the lock and wake up nobody then exist, then
 * thread 1 go to sleep and never get the wake call
 * 
 * NOTE: To test this, we have to introduce a small delay in thread1 between releasing the lock and going to sleep
 * Go to pthread_cond_wait() in pthread.c and add this code
 * 
//   pthread_mutex_unlock(mutex);      // dont actually add this line, just for reference

//   printf("2. thread1 must wait, so it release the lock and about to sleep\n");
//   for (size_t i = 0; i < 900000000; i++)
//   {
    
//   }
//   __syscall(sc_sched_yield, 0x0, 0x0, 0x0, 0x0, 0x0);
//   printf("4. thread1 finally going to sleep now\n");
  
//   *(size_t*) new_waiter_request_to_sleep = 1;        // dont actually add this line, just for reference

    NOTE: since this modify the pthread.c, we should test this cond6 alone.
    NOTE: this test works correctly, if the order of the steps is correct, and theres a delay between step 3 and 4

    WARNING: dont forget to remove the code after testing
*/

#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mymutex6;
pthread_cond_t mycond6;
int sharedData6 = 0;

void* thread1(void* arg)
{
    pthread_mutex_lock(&mymutex6);
    printf("1. thread1 got the lock\n");

    while (sharedData6 == 0) {
        pthread_cond_wait(&mycond6, &mymutex6);
    }

    printf("6. thread1 woken up and just got the lock\n");

    pthread_mutex_unlock(&mymutex6);

    return NULL;
}

void* thread2(void* arg)
{
    for (size_t i = 0; i < 100000000; i++)      // small delay to make sure thread1 is waiting
    {
        /* code */
    }
    
    pthread_mutex_lock(&mymutex6);

    sharedData6 = 42;

    printf("3. thread2 got the lock and is signaling thread1 to wake\n");
    pthread_cond_signal(&mycond6);
    printf("5. thread2 finished and release the lock now\n");

    pthread_mutex_unlock(&mymutex6);

    return NULL;
}

int main()
{
    printf("This test should be tested alone, check the file cond6 for info!\n");
    pthread_t t1, t2;

    pthread_mutex_init(&mymutex6, NULL);
    pthread_cond_init(&mycond6, NULL);

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&mymutex6);
    pthread_cond_destroy(&mycond6);

    return 0;
}