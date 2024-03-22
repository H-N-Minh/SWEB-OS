#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* addition(void* arg) {
    size_t* num = (size_t*) arg;
    return (void*) (num[0] + num[1]);
}

int main() {
    pthread_t thread;
    size_t numbers[2] = {33, 36};

    pthread_create(&thread, NULL, addition, (void*)numbers);

    // create a delay to make sure this thread Join after worker thread finished.
    for (size_t i = 0; i < 100000000; i++)
    {
        if (i % 10000000 == 0)
        {
            printf("Main thread is running\n");
        }
    }
    size_t sum;
    pthread_cancel(thread);
    pthread_join(thread, (void**) &sum);


    printf("Sum of %zu and %zu is: %zu \n", numbers[0], numbers[1], sum);


    return 0;
}
