#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "unistd.h"
#include "sched.h"
#include "nonstd.h"
#include "wait.h"


extern int shared1();
extern int shared2();
extern int shared3();
extern int shared4();
extern int shared5();
extern int shared6();
extern int shared7();
extern int shared8();
extern int shared9();

extern int shared1_anonym();


int SHARED1 = 0;  
int SHARED2 = 0;  
int SHARED3 = 0;  
int SHARED4 = 0;  
int SHARED5 = 0;  
int SHARED6 = 0;  
int SHARED7 = 0;  
int SHARED8 = 0;  
int SHARED9 = 0;  

int SHARED1_ANONYM = 0;  


int childMain()
{
    int retval = 0;

    // comment out the tests you don't want to run
// //******* MAP_SHARED - MAP_ANONYMOUS (using mmap with fork)********

//// NOTE: currently for map_shared, fd are never closed, this should be fixed. this is the only flag that requires writting back to the file, which requires fd to be open for kernel
    // SHARED1 = 1;   // very basic mmap, write and read and munmap, and IPC between 2 processes
    // SHARED2 = 1;   // mmap with read-only protection, child tries to write and should crash
    // SHARED3 = 1;   // mmap with larger size, child writes to every page, Parent should see all the changes made by the child
    // SHARED4 = 1;   // Child calls munmap, parent should still be able to access the page and then call munmap itself on the same page
    // SHARED5 = 1;   // Child calls munmap with too big size (should fail), then with small size (size should be rounded up and page should still be unmapped), 
    //         //          then munmap same page again (should fail). Child shouldnt crash in this test
    // SHARED6 = 1;   // Child calls munmap, then tries to write to page, child should crashes.
    // SHARED7 = 1;   // Calling mmap and munmap with invalid parameters, these should all fail
    SHARED8 = 1;   // Child munmaps the first page, writes to the other 2 pages (to show that they are still accessible), 
    //         //          then munmaps those 2 pages at the same time. child shouldnt crash
    // SHARED9 = 1;   // Child writes to the shared page, then exits without munmap. Parent should still be able to see what the child wrote. 
            //          This shows the page is written back to file (even without munmap) when process terminates


    // SHARED1_ANONYM = 1;   // same as shared1, but uses MAP_ANONYMOUS instead of a file descriptor.


    if (SHARED1)
    {
        printf("\nTesting shared1: very basic mmap, write and read and munmap, and IPC between 2 processes...\n");
        retval = shared1();
        if (retval == 0)                      { printf("===> shared1 successful!\n"); }
        else                                  { printf("===> shared1 failed!\n");  return -1;}
    }
    if (SHARED2)
    {
        printf("\nTesting shared2: read-only protection, child tries to write and should crash...\n");
        retval = shared2();
        if (retval == 0)                      { printf("===> shared2 successful!\n"); }
        else                                  { printf("===> shared2 failed!\n");  return -1;}
    }
    if (SHARED3)
    {
        printf("\nTesting shared3: mmap with larger size...\n");
        retval = shared3();
        if (retval == 0)                      { printf("===> shared3 successful!\n"); }
        else                                  { printf("===> shared3 failed!\n");  return -1;}
    }
    if (SHARED4)
    {
        printf("\nTesting shared4: Child munmap shouldnt affect parents mapping...\n");
        retval = shared4();
        if (retval == 0)                      { printf("===> shared4 successful!\n"); }
        else                                  { printf("===> shared4 failed!\n");  return -1;}
    }
    if (SHARED5)
    {
        printf("\nTesting shared5: invalid and valid munmap size...\n");
        retval = shared5();
        if (retval == 0)                      { printf("===> shared5 successful!\n"); }
        else                                  { printf("===> shared5 failed!\n");  return -1;}
    }
    if (SHARED6)
    {
        printf("\nTesting shared6: Writing after munmap...\n");
        retval = shared6();
        if (retval == 0)                      { printf("===> shared6 successful!\n"); }
        else                                  { printf("===> shared6 failed!\n");  return -1;}
    }
    if (SHARED7)
    {
        printf("\nTesting shared7: Calling mmap and munmap with invalid parameters...\n");
        retval = shared7();
        if (retval == 0)                      { printf("===> shared7 successful!\n"); }
        else                                  { printf("===> shared7 failed!\n");  return -1;}
    }
    if (SHARED8)
    {
        printf("\nTesting shared8: Partial munmap, muiltiple pages munmap...\n");
        retval = shared8();
        if (retval == 0)                      { printf("===> shared8 successful!\n"); }
        else                                  { printf("===> shared8 failed!\n");  return -1;}
    }
    if (SHARED9)
    {
        printf("\nTesting shared9: Testing page being written back to file after process died...\n");
        retval = shared9();
        if (retval == 0)                      { printf("===> shared9 successful!\n"); }
        else                                  { printf("===> shared9 failed!\n");  return -1;}
    }
    
    if (SHARED1_ANONYM)
    {
        printf("\nTesting shared1_anonym: same as shared1, but test with MAP_ANONYMOUS instead of using fd...\n");
        retval = shared1_anonym();
        if (retval == 0)                      { printf("===> shared1_anonym successful!\n"); }
        else                                  { printf("===> shared1_anonym failed!\n");  return -1;}
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
                    