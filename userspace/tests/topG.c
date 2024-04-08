#include <stdio.h>
#include "pthread.h"
#include "assert.h"

// Test if the space reserved for locking is correctly set up in both parent and child thread
// Expected output: Both threads should has top_stack pointing to itself
// TEST: run this test more than once continuously, it should still work
// TODO: Once more pages per threads is implemented, this test should be updated to check if 
//       the top_stack of the new page still points to the first page


void* thread_function(void* arg) {
    size_t* top_stack = (size_t*) getTopOfThisStack();
    assert((size_t) top_stack == (size_t) *top_stack && "top_stack of CHILD is not pointing to itself");
    return NULL;
}


int main() {
    pthread_t thread_id;
    int result = pthread_create(&thread_id, NULL, thread_function, NULL);
    if (result != 0) {
        printf("Failed to create thread.\n");
        return 1;
    }

    size_t* top_stack = (size_t*) getTopOfThisStack();
    assert((size_t) top_stack == (size_t) *top_stack && "top_stack of PARENT is not pointing to itself");

    result = pthread_join(thread_id, NULL);
    if (result != 0) {
        printf("Failed to join thread.\n");
        return 1;
    }

    printf("Test passed.\n");

    return 0;
}