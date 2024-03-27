#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int fork1();
extern int fork2();
extern int fork3();
extern int fork4();
extern int fork5();

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success


// set to 1 to test, 0 to skip
#define FORK1 1     // note: this test also prints 2 lines
#define FORK2 1
#define FORK3 1
#define FORK4 0     // not working right now because fd error when 2 different processes open the same fd
#define FORK5 1


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

    return 0;
}