/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 69]
disabled: false
*/

#include "stdio.h"
#include "unistd.h"
#include "assert.h"
#include "types.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success


// This tests if the 2 processes can have different values of same variable (they should). Both local and global variables are tested.
int global_var = 10;

int main()
{
    size_t x = 0;
    pid_t pid = fork();

    if(pid > 0) // parent
    {
        x += 10;
        global_var += 10;
    }
    else if(pid == 0)   // child
    {
        x += 100;
        global_var += 200;
    }

    x += 50;        // parent and child should have x = 60 and x = 150 respectively
    global_var += 500;

    if (pid > 0)
    {
        assert(x == 60 && "Parent process should have x = 60");
        assert(global_var == 520 && "Parent process should have global_var = 520");
        return PARENT_SUCCESS;
    }
    else
    {
        assert(x == 150 && "Child process should have x = 150");
        assert(global_var == 710 && "Child process should have global_var = 710");
        return CHILD_SUCCESS;
    }
}