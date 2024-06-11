#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


extern int mmap1();
extern int mmap2();
extern int mmap3();

int MMAP1 = 0;  
int MMAP2 = 0;  
int MMAP3 = 0;  


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run
    MMAP1 = 1;   // MAP_PRIVATE | MAP_ANONYMOUS (using mmap like malloc)
    // MMAP2 = 1;   // MAP_SHARED | MAP_ANONYMOUS (using mmap with fork)
    // MMAP3 = 1;   // (not worked on yet) MAP_PRIVATE (using mmap with fd)
    // MMAP4 = 1;   // (not worked on yet) MAP_SHARED (using mmap with fd, multiple processes)


/**
 *  TODO: for MAP_PRIVATE | MAP_ANONYMOUS:
 *      - test giving mmap & munmap invalid parameters (for munmap, length cant be 0, start must be multiple of page size, start must be in the range of mmaped memory)
 *      - test mapping then write, should be possible, then unmap then write again, should be error
 *      - similar to the test above, but 1 threads calls munmap and another thread tries to read afterwards
 *      - test unmaping twice on same address
 *      - test mapping 2 pages then unmap both pages at once using 1 munmap call, this should be possible
 *      - test mapping 1 page but then unmap 2 pages, should be error
 *      - test mapping 2 pages but then unmap 1 pages twice, should be possible
 *      - test mapping on unmapped page.
 *      - changes to the file shall not be visible for other processes (MAP_PRIVATE).
 *      for map p
 *  
 *      
 * */


    if (MMAP1)
    {
        printf("\nTesting mmap1: testing MAP_PRIVATE | MAP_ANONYMOUS (using mmap like malloc)...\n");
        retval = mmap1();
        if (retval == 0)                      { printf("===> mmap1 successful!\n"); }
        else                                  { printf("===> mmap1 failed!\n");  return -1;}
    }

    if (MMAP2)
    {
        printf("\nTesting mmap2: testing MAP_SHARED | MAP_ANONYMOUS (using mmap with fork)...\n");
        retval = mmap2();
        if (retval == 0)                      { printf("===> mmap2 successful!\n"); }
        else                                  { printf("===> mmap2 failed!\n");  return -1;}
    }
    
    printf("\n\n===  All mmap testcases successful  ===\n");
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
                    