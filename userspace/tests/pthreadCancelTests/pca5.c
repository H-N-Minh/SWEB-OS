#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"

void* addition(void* arg) {
    size_t* num = (size_t*) arg;
    return (void*) (num[0] + num[1]);
}

int pca5() {
    pthread_t thread;
    size_t numbers[2] = {33, 36};

    int rv_create = pthread_create(&thread, NULL, addition, (void*)numbers);
    assert(rv_create == 0);
    assert(thread);

    sleep(1);       //delay to ensure addition has finished

    size_t sum;
    int rv_cancel = pthread_cancel(thread);
    assert(rv_cancel != 0);
    int rv_join = pthread_join(thread, (void**) &sum);
    assert(rv_join == 0);
    assert(sum == 69);

    return 0;
}
