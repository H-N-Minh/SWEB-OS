#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"
#include "wait.h"

#define MAX_FORK  64    // should be multiple of 2, so the array can be evenly divided between child processes

#define PAGESIZE 4096
#define PAGES_IN_ARRAY 2000 
#define ELEMENTS_IN_ARRAY (PAGES_IN_ARRAY * PAGESIZE)/8        


size_t big_array[ELEMENTS_IN_ARRAY];
size_t pids[MAX_FORK];


// Test: Many forks and children trigger out of memory
int swappingFork2() 
{  
    size_t x = 0;
    for (int i = 0; i < MAX_FORK; i++) {
        assert(x == i  && "each process should have its own unique value of x");
        pid_t pid = fork();
        pids[i] = pid;
    
        if (pid > 0) 
        {
            x += 1;      
            continue;    // parent continues to fork
        } 
        else if (pid == 0) 
        {   // child writes data to array then die
            int start = i * (PAGES_IN_ARRAY / MAX_FORK);
            int end = (i + 1) * (PAGES_IN_ARRAY / MAX_FORK);
            for(int i2 = start; i2 < end; i2++)
            {
                big_array[i2 * (PAGESIZE / 8)] = (size_t)i2;
            }
            for(int i3 = start; i3 < end; i3++)
            {
                assert(big_array[i3 * (PAGESIZE / 8)] == i3);
            }
            exit(0);
        } 
        else 
        {
            assert(0);
        }
    }

    for(int i = 0; i < MAX_FORK; i++)
    {
        int status = 0;
        waitpid(pids[i], &status, 0);
        if (status != 0)
        {
            assert(0 && "Child exits with wrong value.\n");
        }
    }


    // only 1 last process would reach here
    assert(x == MAX_FORK  && "parent process (the 100th process) should now have x = 100");
    
    return 0;
}