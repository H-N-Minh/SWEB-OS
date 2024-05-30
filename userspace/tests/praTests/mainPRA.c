#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


extern int pra1();
extern int pra2();
extern int pra3();
extern int random1();

int PRA1 = 0;  
int PRA2 = 0; 
int PRA3 = 0;  
int RANDOM1 = 0;

// @problem: currently pra1 and pra2 cant be run together


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run
    // PRA1 = 1;   // Test Random PRA
    // PRA2 = 1;   // Test NFU PRA
    // PRA3 = 1;      // Test switching PRA every 100 pages using syscall
    RANDOM1 = 1;  // Test Random PRA is actually random

    if (PRA1)
    {
        printf("\nTesting pra1: testing Random-PRA...\n");
        retval = pra1();
        if (retval == 0)                      { printf("===> pra1 successful!\n"); }
        else                                  { printf("===> pra1 failed!\n");  return -1;}
    }

    if (PRA2)
    {
        printf("\nTesting pra2: testing NFU-PRA...\n");
        retval = pra2();
        if (retval == 0)                      { printf("===> pra2 successful!\n"); }
        else                                  { printf("===> pra2 failed!\n");  return -1;}
    }

    if (PRA3)
    {
        printf("\nTesting pra3: test switching PRA using syscall...\n");
        retval = pra3();
        if (retval == 0)                      { printf("===> pra3 successful!\n"); }
        else                                  { printf("===> pra3 failed!\n");  return -1;}
    }

    if (RANDOM1)
    {
        printf("\nTesting random1: testing Random PRA is actually random...\n");
        retval = random1();
        if (retval == 0)                      { printf("===> random1 successful!\n"); }
        else                                  { printf("===> random1 failed!\n");  return -1;}
    }


    printf("\n\n===  All pra testcases successful  ===\n");
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
                    