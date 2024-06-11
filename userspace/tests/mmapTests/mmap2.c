#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int mmap2() {
    void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
    {
        exit(1);
    }
    printf("mmap done, writting to mapped page\n");

    int *num = (int*) addr;
    *num = 10;
    printf("done writting, forking now\n");
    if (fork() == 0)
    {
        // This is the child process.
        printf("child starts\n");
        printf("Child sees *num = %d\n", *num);
        *num = 20;
        printf("Child changed *num to 20\n");
        exit(0);
    } 
    else
    {
        printf("parent starts\n");
        // This is the parent process.
        sleep(1);  // Sleep for a second to let the child run.
        if (*num != 20)
        {
            printf("Error: Parent sees *num = %d, but it should be 20\n", *num);
            return -1;
        }
        printf("Parent sees *num = %d\n", *num);

    }

    return 0;
}
