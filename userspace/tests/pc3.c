#include "pthread.h"
#include <assert.h>


int main()
{
    printf("pc3 successful if no assertion gets raised!\n");
    int value;
    //Test1: Pthread_exit in main
    pthread_exit(&value);
    assert(0 && "pthread_exit with only one thread left should terminate the process");
    return 0;
}