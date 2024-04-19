#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int fork1();
extern int fork2();
extern int fork3();
extern int fork4();
extern int fork5();
extern int fork6();

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

// TODO: test fork while holding a lock

// set to 1 to test, 0 to skip
// all 4 & 5 & 6 requires lot of physical memory so each should be tested alone
#define FORK1 1     //simple fork
#define FORK2 1     // This tests if the 2 processes can have different values of same variable (they should). Both local and global variables are tested
#define FORK3 1     // this tests fork together with pthread_create
#define FORK4 1     // this tests multiple nested forks
#define FORK5 1     // this tests multiple nested forks
#define FORK6 1     // test the locking of archmemory (NOTE: not a really good test for locking)

int main()
{
    int retval = 0;

    if (FORK1)
    {
        retval = fork1();
        if (retval == PARENT_SUCCESS)         { printf("fork1 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return 0; }                      // this kills all child processes
        else                                  { printf("fork1 failed!\n");  return -1;}
    }

    if (FORK2)
    {
        retval = fork2();
        if (retval == PARENT_SUCCESS)         { printf("fork2 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return 0; }                      
        else                                  { printf("fork2 failed!\n"); return -1;}
    }

    if (FORK3)
    {
        retval = fork3();
        if (retval == PARENT_SUCCESS)         { printf("fork3 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return 0; }                      
        else                                  { printf("fork3 failed!\n"); return -1;}
    }

    if (FORK4)
    {
        retval = fork4();
        if (retval == PARENT_SUCCESS)         { printf("fork4 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return 0; }                      
        else                                  { printf("fork4 failed!\n"); return -1;}
    }

    if (FORK5)
    {
        retval = fork5();
        if (retval == PARENT_SUCCESS)         { printf("fork5 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return 0; }                      
        else                                  { printf("fork5 failed!\n"); return -1;}
    }

    if (FORK6)
    {
        retval = fork6();
        if (retval == PARENT_SUCCESS)         { printf("fork6 successful!\n"); }
        else if (retval == CHILD_SUCCESS)     { return 0; }
        else                                  { printf("fork6 failed!\n"); return -1;}
    }

    return 0;
}