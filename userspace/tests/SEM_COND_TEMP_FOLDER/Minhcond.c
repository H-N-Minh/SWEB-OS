#include <stdio.h>
#include <pthread.h>

typedef struct {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} condition_variable_t;

void condition_variable_init(condition_variable_t* cv) {
    cv->value = 0;
    pthread_mutex_init(&cv->mutex, NULL);
    pthread_cond_init(&cv->cond, NULL);
}

void condition_variable_wait(condition_variable_t* cv) {
    pthread_mutex_lock(&cv->mutex);
    while (cv->value == 0) {
        pthread_cond_wait(&cv->cond, &cv->mutex);
    }
    cv->value = 0;
    pthread_mutex_unlock(&cv->mutex);
}

void condition_variable_signal(condition_variable_t* cv) {
    pthread_mutex_lock(&cv->mutex);
    cv->value = 1;
    pthread_cond_signal(&cv->cond);
    pthread_mutex_unlock(&cv->mutex);
}

void condition_variable_destroy(condition_variable_t* cv) {
    pthread_mutex_destroy(&cv->mutex);
    pthread_cond_destroy(&cv->cond);
}

// Example usage
condition_variable_t cv;

void* thread_func(void* arg) {
    printf("Thread waiting...\n");
    condition_variable_wait(&cv);
    printf("Thread resumed!\n");
    return NULL;
}

int main() {
    pthread_t thread;
    condition_variable_init(&cv);

    pthread_create(&thread, NULL, thread_func, NULL);

    // Allow some time for the thread to start and wait
    sleep(1);

    printf("Signaling the condition variable...\n");
    condition_variable_signal(&cv);

    pthread_join(thread, NULL);
    condition_variable_destroy(&cv);

    return 0;
}