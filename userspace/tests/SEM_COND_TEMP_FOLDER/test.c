#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int flag = 0;

void* thread_func(void* arg) {
    pthread_mutex_lock(&mutex);
    while (flag == 0) {
        pthread_cond_wait(&cond, &mutex);
    }
    printf("Thread woke up!\n");
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&thread, NULL, thread_func, NULL);

    // Simulating some work
    printf("Main thread is doing some work...\n");
    sleep(2);

    pthread_mutex_lock(&mutex);
    flag = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_join(thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}