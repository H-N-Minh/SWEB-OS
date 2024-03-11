#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* addition(void* arg) {
    size_t* num = (size_t*) arg;
    return (void*) (num[0] + num[1]);
}

int main() {
    pthread_t thread;
    pthread_t thread2;
    size_t numbers[2] = {10, 20};

    pthread_create(&thread, NULL, addition, (void*)numbers);

    while (1)
    {
        /* code */
    }
    

    size_t* sum;
    pthread_join(thread, (void*) &sum);
    

    printf("Sum of %zu and %zu is: %zu\n", numbers[0], numbers[1], *sum);

    return 0;
}
