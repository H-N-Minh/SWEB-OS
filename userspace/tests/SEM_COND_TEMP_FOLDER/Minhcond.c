#include "Minhcond.h"


void Minh_pthread_cond_init(Minh_pthread_cond_t* cv, const pthread_condattr_t *attr) {
    cv->value = 0;
    // pthread_mutex_init(&cv->mutex, NULL);
}


void Minh_pthread_cond_wait(Minh_pthread_cond_t* cv, pthread_mutex_t* user_mutex) {
    // // pthread_mutex_lock(&cv->mutex); ???
    // assert(cv->value == 0); // request for sleep should be 0 before calling wait
    // cv->request_to_sleep_ = 1;  // so scheduler set this CURRENT thread to sleep (use calculation here to find request_to_sleep_ address)
    // sleeper_.push_back(cv->linkedlistaddress);  // add address to list so signal can change it back to 0
    // pthread_mutex_unlock(user_mutex);       // lost wake call
    // Scheduler::yield();
    // pthread_mutex_lock(user_mutex);
    // // pthread_mutex_unlock(&cv->mutex); ??
}

void Minh_pthread_cond_signal(Minh_pthread_cond_t* cv) {
    // // pthread_mutex_lock(&cv->mutex);???
    // if (!sleepers_.empty()) 
    // { 
    //     sleepers_.front()->request_to_sleep_ = 0; 
    //     sleepers_.pop_front(); 
    // } 
    // // pthread_mutex_unlock(&cv->mutex);????
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