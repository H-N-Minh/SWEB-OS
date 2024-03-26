#include "stdio.h"
#include "unistd.h"
#include "assert.h"

extern int fork1();
extern int fork2();

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success


// set to 1 to test, 0 to skip
#define FORK1 1
#define FORK2 1

//NOTE: only test 1 at a time, because after 1 fork, process will be duplicated and 2 processes will return
//      and run this main func 
int main()
{
    int retval = 0;

    if (FORK1)
    {
        retval = fork1();
        if (retval == PARENT_SUCCESS)         { printf("fork1 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return 0; }                      
        else                                  { printf("fork1 failed!\n");  }
    }

    if (FORK2)
    {
        retval = fork2();
        if (retval == PARENT_SUCCESS)         { printf("fork2 successful!\n"); } 
        else if (retval == CHILD_SUCCESS)     { return 0; }                      
        else                                  { printf("fork2 failed!\n");  }
    }

    return 0;
}