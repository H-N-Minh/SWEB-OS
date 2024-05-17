#include "stdio.h"
#include "assert.h"
#include "unistd.h"
#include "wait.h"
#include "pthread.h"

extern int fork1();
extern int fork2();
extern int fork3();
extern int fork4();
extern int fork5();
extern int fork6();
extern int fork7();

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

// TODO: test fork while holding a lock

// set to 1 to test, 0 to skip
// all 4 & 5 & 6 requires lot of physical memory so each should be tested alone
#define FORK1 0     //simple fork
#define FORK2 0     // This tests if the 2 processes can have different values of same variable (they should). Both local and global variables are tested
#define FORK3 0     // this tests fork then each calls pthread_create
#define FORK4 0     // this tests multiple nested forks (each child will fork again)
#define FORK5 0     // this tests multiple nested forks (same thread will fork multiple times)
#define FORK6 0     // 100 pcreate then all fork at same time
#define FORK7 1     // fork with waitpid

int childMain()
{
    int retval = 0;

    if (FORK1)
    {
        printf("\nTesting fork1: basic test...\n");
        retval = fork1();
        if (retval == PARENT_SUCCESS)         { printf("===> fork1 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return CHILD_SUCCESS; }                      // this kills all child processes
        else                                  { printf("===> fork1 failed!\n");  return -1;}
    }

    if (FORK2)
    {
        printf("\nTesting fork2: 2 processes should have their own memory space...\n");
        retval = fork2();
        if (retval == PARENT_SUCCESS)         { printf("===> fork2 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return CHILD_SUCCESS; }                      
        else                                  { printf("===> fork2 failed!\n"); return -1;}
    }

    if (FORK3)
    {
        printf("\nTesting fork3: fork and then pcreate...\n");
        retval = fork3();
        if (retval == PARENT_SUCCESS)         { printf("===> fork3 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return CHILD_SUCCESS; }                      
        else                                  { printf("===> fork3 failed!\n"); return -1;}
    }

    if (FORK4)
    {
        printf("\nTesting fork4: recursive fork (each child forks again)...\n");
        retval = fork4();
        if (retval == PARENT_SUCCESS)         { printf("===> fork4 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return CHILD_SUCCESS; }                      
        else                                  { printf("===> fork4 failed!\n"); return -1;}
    }

    if (FORK5)
    {
        printf("\nTesting fork5: recursive fork (same parent fork multiple child)...\n");
        retval = fork5();
        if (retval == PARENT_SUCCESS)         { printf("===> fork5 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return CHILD_SUCCESS; }                      
        else                                  { printf("===> fork5 failed!\n"); return -1;}
    }

    if (FORK6)
    {
        printf("\nTesting fork6: 100 pcreate then all fork at same time...\n");
        retval = fork6();
        if (retval == PARENT_SUCCESS)         { printf("===> fork6 successful!\n"); }
        else if (retval == CHILD_SUCCESS)     { return CHILD_SUCCESS; }
        else                                  { printf("===> fork6 failed!\n"); return -1;}
    }

    if (FORK7)
    {
        printf("\nTesting fork7: fork with waitpid...\n");
        retval = fork7();
        if (retval == PARENT_SUCCESS)         { printf("===> fork7 successful!\n"); }
        else if (retval == CHILD_SUCCESS)     { return CHILD_SUCCESS; }
        else                                  { printf("===> fork7 failed!\n"); return -1;}
    }

    printf("\n\n===  All fork testcases successful  ===\n");
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
        if (status == CHILD_SUCCESS)
        {
            return 0;
        }
        
        else if (status != 0)
        {
            printf("Testing crashed with exit code %d\n", status);
            return -1;
        }

        for (size_t i = 0; i < 200000000; i++)      // give some time for all threads to die
        {
            /* code */
        }

        int num = get_thread_count();
        if (num == 7 || num == 6)
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
                    