/**
 * This test the "lost wake call", which is when a thread is holding a lock, about to go to sleep so another thread
 * can take that lock and wake first thread up
 * Optimal is thread1 release the lock and go to sleep, thread2 take the lock and wake up thread1
 * problem is when thread1 release the lock, thread 2 take the lock and wake up nobody then exist, then
 * thread 1 go to sleep and never get the wake call
*/

int cond6()
{
    // pthread_t thread1, thread2;
    // pthread_mutex_t mutex;
    // pthread_cond_t cond;
    // int i = 0;
    int retval = 0;

    // pthread_mutex_init(&mutex, NULL);
    // pthread_cond_init(&cond, NULL);

    // pthread_create(&thread1, NULL, &cond6_thread1, &cond);
    // pthread_create(&thread2, NULL, &cond6_thread2, &cond);

    // pthread_join(thread1, NULL);
    // pthread_join(thread2, NULL);

    // pthread_mutex_destroy(&mutex);
    // pthread_cond_destroy(&cond);

    return retval;
}