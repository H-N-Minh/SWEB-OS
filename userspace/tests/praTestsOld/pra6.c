#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"
#include "wait.h"
#include "nonstd.h"

#define PARENT_SUCCESS 0    // parent process returns 0 on success
#define CHILD_SUCCESS 69    // child process returns 69 on success

#define MAX_FORK  64    // should be multiple of 2, so the array can be evenly divided between child processes


#define MEGABYTE 1048576    // 1mb
#define PAGESIZE 4096

#define N 5
#define N_MEGABYTE N * MEGABYTE


#define ELEMENTS_IN_ARRAY N_MEGABYTE / 8              
#define PAGES_IN_ARRAY N_MEGABYTE/PAGESIZE            


size_t big_array6[ELEMENTS_IN_ARRAY];  //5 Megabyes

// this tests multiple nested forks. 
// Each child before dying writes to a part of the big array.
// The amount of each write is not big so a child cant trigger swapping by itself, but
// Each child occupying a part of RAM will eventually trigger swapping
int pra6() 
{  
    setPRA(__NFU_PRA__); 
    int hit;
    int miss;
    getPRAstats(&hit, &miss);
    printf("NFU PRA: Hit: %d, Miss: %d (before test)\n", hit, miss);

    int child_pids[MAX_FORK];
    size_t x = 0;
    for (int i = 0; i < MAX_FORK; i++) {
        assert(x == i  && "each process should have its own unique value of x");
        pid_t pid = fork();
        if (pid > 0) 
        {
            x += 1;      
            child_pids[i] = pid;
            continue;    // parent continues to fork
        } 
        else if (pid == 0) 
        {   // child writes data to array then die
            int start = i * (PAGES_IN_ARRAY / MAX_FORK);
            int end = (i + 1) * (PAGES_IN_ARRAY / MAX_FORK);
            for(int i2 = start; i2 < end; i2++)
            {
                big_array6[i2 * (PAGESIZE / 8)] = (size_t)i2;
            }
            for(int i3 = start; i3 < end; i3++)
            {
                assert(big_array6[i3 * (PAGESIZE / 8)] == i3);
            }
            exit(CHILD_SUCCESS);        // kill child process
        } 
        else 
        {
            return -1;
        }
    }

    // printf("Parent process done, waiting for all childs\n");
    for (size_t i = 0; i < MAX_FORK; i++)
    {
        int status;
        waitpid(child_pids[i], &status, 0);
        if (status != CHILD_SUCCESS)
        {
            printf("Child process %zu failed with status %d\n", i, status);
            return -1;
        }
    }
    // printf("all childs finished, parent checking the array\n");
    for(int i = 0; i < PAGES_IN_ARRAY; i++)
    {
        if (big_array6[i * (PAGESIZE / 8)] != i)
        {
        printf("Error: big_array6[%ld] != i: %d\n", big_array6[i * (PAGESIZE / 8)], i);
        }
        assert(big_array6[i * (PAGESIZE / 8)] == i);
    }
    
    // only 1 last process would reach here
    assert(x == MAX_FORK  && "parent process (the 100th process) should now have x = 100");

    getPRAstats(&hit, &miss);
    printf("NFU PRA: Hit: %d, Miss: %d (after test)\n", hit, miss);
    
    return 0;
}