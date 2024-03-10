#include "stdio.h"
#include "pthread.h"
#include <assert.h>


void add6900(size_t x)
{
    size_t y = x + 6900;
    printf("Hello from add6900 func, y is now %zu\n", y);
}

int main()
{
    size_t x = 69;
    pthread_create(NULL, NULL, (void* (*)(void*))add6900, (void*) x);

    return 0;
}