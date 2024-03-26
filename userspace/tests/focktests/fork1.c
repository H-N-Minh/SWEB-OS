#include "stdio.h"
#include "unistd.h"
#include "assert.h"



int fork1()
{
    pid_t pid = fork();
    size_t x = 0;

    if(pid > 0)
    {
        x += 10;
    }
    else if(pid == 0)
    {
        x += 100;
    }

    x += 50;
    if (pid == 0)
    {
        assert(x == 150 && "Child process should have x = 150");
        return 69;
    }
    else
    {
        assert(x == 60 && "Parent process should have x = 60");
        return 0;
    }

}