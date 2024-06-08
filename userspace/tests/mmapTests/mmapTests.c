#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


extern int mmap1();

int MMAP1 = 0;  


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run
    MMAP1 = 1;   // using mmap like malloc (no fd)


    if (MMAP1)
    {
        printf("\nTesting mmap1: testing using mmap like malloc (no fd)...\n");
        retval = mmap1();
        if (retval == 0)                      { printf("===> mmap1 successful!\n"); }
        else                                  { printf("===> mmap1 failed!\n");  return -1;}
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
                    