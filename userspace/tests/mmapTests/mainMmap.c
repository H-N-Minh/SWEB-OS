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
extern int mmap4();

extern int private1();
extern int private1_anonym();

int MMAP1 = 0;  
int MMAP2 = 0;  
int MMAP3 = 0;  
int MMAP4 = 0;  

int PRIVATE1 = 0;
int PRIVATE1_ANONYM = 0;


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run

//******* MAP_PRIVATE | MAP_ANONYMOUS (using mmap like malloc)********
    PRIVATE1 = 1;   // Collection of bunch of basic tests for MAP_PRIVATE
    PRIVATE1_ANONYM = 1;   // same as private1, but uses MAP_ANONYMOUS instead of a file descriptor.

// **** completed tests, but still needs work
    // MMAP1 = 1;   // MAP_PRIVATE | MAP_ANONYMOUS (using mmap like malloc)
    // MMAP2 = 1;   // MAP_SHARED | MAP_ANONYMOUS (using mmap with fork)
    // MMAP3 = 1;   // MAP_PRIVATE (using mmap with fd)
    // MMAP4 = 1;   // MAP_SHARED (using mmap with fd, multiple processes)


/** tests ideas:
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
 *      - map private a page, then call fork. each process should now has its own version of the page. (cow)
 * 
 *      
 *      for MAP_PRIVATE:
 *      - wrong params
 *      - check writing back to file successfully with and without munmap
 *      - open file that has 5 pages, mmap with offset 1 page, reading data from page 2 3 4 5 should be correct. Also try reading page 5 first then 2 then 4 then 3
 *      - write to mem, unmap, check if the data still written to the file. Also try this with process termination instead of unmap
 *      - mmap then fork, both process should have their own version of the page privately
 *      
 *     for general:
 *      - mmap with wrong params
 *      - mmap with multiple pages
 *      - give a bullshit fd value
 *     - test different protection flags
 *      - test different flags
 * 
 *      - set readonly prot then write to it
 *      - writting/reading after munmap
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
        printf("\nTesting mmap2: testing MAP_SHARED | MAP_ANONYMOUS (using mmap with fork and IPC)...\n");
        retval = mmap2();
        if (retval == 0)                      { printf("===> mmap2 successful!\n"); }
        else                                  { printf("===> mmap2 failed!\n");  return -1;}
    }
    
    if (MMAP3)
    {
        printf("\nTesting mmap3: testing MAP_PRIVATE (using mmap with open() and fd)...\n");
        retval = mmap3();
        if (retval == 0)                      { printf("===> mmap3 successful!\n"); }
        else                                  { printf("===> mmap3 failed!\n");  return -1;}
    }
    
    if (MMAP4)
    {
        printf("\nTesting mmap4: testing MAP_PRIVATE (using mmap with open() and fd)...\n");
        retval = mmap4();
        if (retval == 0)                      { printf("===> mmap4 successful!\n"); }
        else                                  { printf("===> mmap4 failed!\n");  return -1;}
    }
    
    if (PRIVATE1)
    {
        printf("\nTesting private1: testing bunch of basic tests for map_private...\n");
        retval = private1();
        if (retval == 0)                      { printf("===> private1 successful!\n"); }
        else                                  { printf("===> private1 failed!\n");  return -1;}
    }
    
    if (PRIVATE1_ANONYM)
    {
        printf("\nTesting private1_anonym: same as private1, but test with MAP_ANONYMOUS...\n");
        retval = private1_anonym();
        if (retval == 0)                      { printf("===> private1_anonym successful!\n"); }
        else                                  { printf("===> private1_anonym failed!\n");  return -1;}
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
                    