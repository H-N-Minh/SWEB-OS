#include "Minhcond.h"


void Minh_pthread_cond_init(Minh_pthread_cond_t* cv, const pthread_condattr_t *attr) {
    cv->value = 0;
    pthread_mutex_init(&cv->mutex, NULL);
}

void Minh_pthread_cond_wait(Minh_pthread_cond_t* cv, pthread_mutex_t* given_mutex) {
    pthread_mutex_unlock(given_mutex);
    pthread_mutex_lock(&cv->mutex);
    while (cv->value == 0) {
        pthread_mutex_unlock(&cv->mutex);
        pthread_mutex_lock(&cv->mutex);
    }
    cv->value = 0;
    pthread_mutex_unlock(&cv->mutex);
    pthread_mutex_lock(given_mutex);
}

void Minh_pthread_cond_signal(Minh_pthread_cond_t* cv) {
    pthread_mutex_lock(&cv->mutex);
    cv->value = 1;
    pthread_mutex_unlock(&cv->mutex);
}

void Minh_pthread_cond_destroy(Minh_pthread_cond_t* cv) {
    pthread_mutex_destroy(&cv->mutex);
}

// // Example usage
// Minh_pthread_cond_t cv;

// void* thread_func(void* arg) {
//     printf("Thread waiting...\n");
//     Minh_pthread_cond_wait(&cv);
//     printf("Thread resumed!\n");
//     return NULL;
// }

// int main() {
//     pthread_t thread;
//     Minh_pthread_cond_init(&cv);

//     pthread_create(&thread, NULL, thread_func, NULL);

//     // Allow some time for the thread to start and wait
//     sleep(5);

//     printf("Signaling the condition variable...\n");
//     Minh_pthread_cond_signal(&cv);

//     pthread_join(thread, NULL);
//     Minh_pthread_cond_destroy(&cv);

//     return 0;
// }