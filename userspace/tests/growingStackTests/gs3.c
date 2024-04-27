#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "pthread.h"

#define PAGE_SIZE3 4096
#define THREADS3 2

// Function to be executed by the new thread
void* threadFunc(void* arg)
{
        int i = 0;
        int *p = &i;

        p =(int*) ((size_t)p - PAGE_SIZE3);
        *p = 11;
        printf("1 page added, currently 2 pages total\n");

        p =(int*) ((size_t)p - PAGE_SIZE3);
        *p = 22;
        printf("2 page added, currently 3 pages total\n");

        p =(int*) ((size_t)p - PAGE_SIZE3);
        *p = 33;
        printf("3 page added, currently 4 pages total\n");

        return NULL;
}

// Test a very basic growing stack
int gs3()
{
        int i = 0;
        int *p = &i;

        p =(int*) ((size_t)p - PAGE_SIZE3);
        *p = 11;
        printf("1 page added, currently 2 pages total\n");

        p =(int*) ((size_t)p - PAGE_SIZE3);
        *p = 22;
        printf("2 page added, currently 3 pages total\n");

        p =(int*) ((size_t)p - PAGE_SIZE3);
        *p = 33;
        printf("3 page added, currently 4 pages total\n");

        // Create a new thread
        pthread_t thread;
        pthread_create(&thread, NULL, threadFunc, NULL);

        // Wait for the new thread to finish
        pthread_join(thread, NULL);

        return 0;
}
