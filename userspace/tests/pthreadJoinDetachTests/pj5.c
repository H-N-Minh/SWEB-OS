#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "assert.h"

void* addition(void* arg) {
    size_t* num = (size_t*) arg;
    return (void*) (num[0] + num[1]);
}

//Simple pthread join with returnvalue
int pj5() 
{
    pthread_t thread;
    size_t numbers[2] = {33, 36};

    int rv_create = pthread_create(&thread, NULL, addition, (void*)numbers);
    assert(rv_create == 0);
    assert(thread);

    size_t sum;
    int rv_join = pthread_join(thread, (void**) &sum);
    
    assert(rv_join == 0);
    assert(sum == 69);
    //printf("Sum of %zu and %zu is: %zu \n", numbers[0], numbers[1], sum);
    return 0;
}
