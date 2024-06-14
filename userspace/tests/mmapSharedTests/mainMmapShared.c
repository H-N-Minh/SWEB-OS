#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


extern int shared1();
extern int shared1_anonym();


int SHARED1 = 0;  
int SHARED1_ANONYM = 0;  


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run

// //******* MAP_SHARED - MAP_ANONYMOUS (using mmap with fork)********
//// NOTE: currently for map_shared, fd are never closed, this should be fixed. this is the only flag that requires writting back to the file, which requires fd to be open for kernel
    SHARED1 = 1;   // Collection of bunch of basic tests for MAP_SHARED
    // SHARED1_ANONYM = 1;   // same as shared1, but uses MAP_ANONYMOUS instead of a file descriptor.


    if (SHARED1)
    {
        printf("\nTesting shared1: testing bunch of basic tests for MAP_SHARED...\n");
        retval = shared1();
        if (retval == 0)                      { printf("===> shared1 successful!\n"); }
        else                                  { printf("===> shared1 failed!\n");  return -1;}
    }
    
    if (SHARED1_ANONYM)
    {
        printf("\nTesting shared1_anonym: same as shared1, but test with MAP_ANONYMOUS instead of using fd...\n");
        retval = shared1_anonym();
        if (retval == 0)                      { printf("===> shared1_anonym successful!\n"); }
        else                                  { printf("===> shared1_anonym failed!\n");  return -1;}
    }
    
    printf("\n\n===  All mmap testcases successful  ===\n");
    return 0;
}

int main()
{
    pid_t pid = fork();
    if (pid == 0) {
        int child_exit_code = childMain();
        exit(child_exit_code);
    } 
    else
    {
        int status;
        waitpid(pid, &status, 0);
        
        if (status != 0)
        {
            printf("Testing crashed with exit code %d\n", status);
            return -1;
        }

        for (size_t i = 0; i < 200000000; i++)      // give some time for all threads to die
        {
            /* code */
        }

        int num = get_thread_count();
        if (num == 8 || num == 7)
        {
            printf("===  All threads are destroyed correctly  ===\n");
            return 0;
        }
        else
        {
            printf("===  %d threads are still alive===  \n", num);
            return -1;
        }
        
    }
    return 0;
}
                    