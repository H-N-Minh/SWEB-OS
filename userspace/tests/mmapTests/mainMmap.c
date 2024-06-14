#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


extern int shared1();
extern int shared1_anonym();

extern int private1();
extern int private1_anonym();

int SHARED1 = 0;  
int SHARED1_ANONYM = 0;  

int PRIVATE1 = 0;
int PRIVATE1_ANONYM = 0;


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run

//******* MAP_PRIVATE - MAP_ANONYMOUS (using mmap like malloc)********
    // PRIVATE1 = 1;   // Collection of bunch of basic tests for MAP_PRIVATE
    // PRIVATE1_ANONYM = 1;   // same as private1, but uses MAP_ANONYMOUS instead of a file descriptor.

// //******* MAP_SHARED - MAP_ANONYMOUS (using mmap with fork)********
//     // NOTE: currently for map_shared, fd are never closed, this should be fixed. this is the only flag that requires writting back to the file, which requires fd to be open for kernel
    // SHARED1 = 1;   // Collection of bunch of basic tests for MAP_SHARED
    SHARED1_ANONYM = 1;   // same as shared1, but uses MAP_ANONYMOUS instead of a file descriptor.


/** tests ideas:
 *  TODO:
 *      for MAP_SHARED - MAP_ANONYMOUS:
 *      - mmap 3 pages and write to all 3, then munmap all 3, then mmap again the same fd but with offset 1 page. read the data to see if the page mapped at the right offet 
 *      
 *      for MAP_PRIVATE - MAP_ANONYMOUS:
 *      - test munmap on unmapped page.
 *      - set read-only prot then write to it. should crash
 *      - writting/reading after munmap. should crash
 *      - mmap then fork, both process should have their own version of the page privately (child writes st to the page then die. parent shouldnt see the change)
 * 
 *      for both MAP_PRIVATE and MAP_SHARED:
 *      - open file that has 5 pages. Try writing/reading randomly (zb: page 5 first then 2 then 4 then 3). should work
 *      - write to file, unmap, then mmap again with the same process. check if the data still written to the file. data should still be there
 *      - first mmap 1 page then mmap 1 page again. then calls munmap using address of the first mmap, but with size of 2 pages. this should be possible
 *      - calling munmap using invalid address: address that is not page aligned, address that is not in the range of mmaped memory. should fail
 *      - mmap 3 pages, calling munmap on the second page. 1st and 3rd page should still be accessible 
 *      
 * */

    if (SHARED1)
    {
        printf("\nTesting shared1: testing bunch of basic tests for MAP_SHARED...\n");
        retval = shared1();
        if (retval == 0)                      { printf("===> shared1 successful!\n"); }
        else                                  { printf("===> shared1 failed!\n");  return -1;}
    }
    
    if (SHARED1_ANONYM)
    {
        printf("\nTesting shared1_anonym: same as shared1, but test with MAP_ANONYMOUS instead of using fd...\n");
        retval = shared1_anonym();
        if (retval == 0)                      { printf("===> shared1_anonym successful!\n"); }
        else                                  { printf("===> shared1_anonym failed!\n");  return -1;}
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
        printf("\nTesting private1_anonym: same as private1, but test with MAP_ANONYMOUS instead of using fd...\n");
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
                    