#include "pthread.h"
#include "assert.h"
#include "unistd.h"

int flag = 0;
void sleeper_function()
{
    flag = 1;
    sleep(100);
}

//sleep and exit
int main()  
{
    pthread_t thread_id;
    int rv = pthread_create(&thread_id, NULL, (void * (*)(void *)) sleeper_function, NULL);
    while(!flag){}
    sleep(1);
    return 0;
}