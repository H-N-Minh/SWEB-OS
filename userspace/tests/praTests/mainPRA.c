#include "stdio.h" //
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
extern int pra4();
extern int pra5();
extern int pra6();

int PRA1 = 0;  
int PRA2 = 0; 
int PRA3 = 0;  
int RANDOM1 = 0;
int PRA4 = 0;
int PRA5 = 0;
int PRA6 = 0;
// @problem: currently pra1 and pra2 cant be run together


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run
    PRA1 = 1;   // Test PRA
    // PRA2 = 1;   // Test PRA
    // PRA3 = 1;      // Test switching PRA every 100 pages using syscall
    // RANDOM1 = 1;  // Test Random PRA is actually random

    // currently our PRA is not passing these 3 tests
    PRA4 = 1;       // double pagefault => double swap (still single thread)
    PRA5 = 1;       // 64 threads  writting to array at same time
    PRA6 = 1;       // similar to pra5, but use fork instead of pthread_create

    // @todo add test that shows NFU is better than Random
    // setPRA(__RANDOM_PRA__); 
    // setPRA(__NFU_PRA__); 
    setPRA(__SECOND_CHANCE_PRA__); 

    if (PRA1)
    {
        printf("\nTesting pra1: testing Random-PRA...\n");
        retval = pra1();
        if (retval == 0)                      { printf("===> pra1 successful!\n"); }
        else                                  { printf("===> pra1 failed!\n");  return -1;}
    }

    // if (PRA2)
    // {
    //     printf("\nTesting pra2: testing NFU-PRA...\n");
    //     retval = pra2();
    //     if (retval == 0)                      { printf("===> pra2 successful!\n"); }
    //     else                                  { printf("===> pra2 failed!\n");  return -1;}
    // }

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

    if (PRA4)
    {
        printf("\nTesting pra4: testing double pagefault => double swap...\n");
        retval = pra4();
        if (retval == 0)                      { printf("===> pra4 successful!\n"); }
        else                                  { printf("===> pra4 failed!\n");  return -1;}
    }

    if (PRA5)
    {
        printf("\nTesting pra5: testing 64 threads writing to array in parallel...\n");
        retval = pra5();
        if (retval == 0)                      { printf("===> pra5 successful!\n"); }
        else                                  { printf("===> pra5 failed!\n");  return -1;}
    }

    if (PRA6)
    {
        printf("\nTesting pra6: testing swapping in combination with fork...\n");
        retval = pra5();
        if (retval == 0)                      { printf("===> pra5 successful!\n"); }
        else                                  { printf("===> pra5 failed!\n");  return -1;}
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
                    