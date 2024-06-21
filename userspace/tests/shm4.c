#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "assert.h"
#include <string.h>   // for strcpy
#include <fcntl.h>    // for open
#include "wait.h"

#define SHM_SIZE 4096
#define SHM_NAME "/my_shared_memory"

int main() {
    int shm_fd;
    void *shm_ptr;
    pid_t pid;

    // Create a shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        exit(1);
    }

    printf("Shared memory object created with fd: %d\n", shm_fd);

    // Map the shared memory object into the address space of the parent process
    shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        exit(1);
    }
    printf("Shared memory object mapped to the address: %p\n", shm_ptr);

    // Fork a child process
    pid = fork();
    if (pid == -1) {
        exit(1);
    }

    if (pid == 0) {
        printf("Child process starting\n");
        // Child process
        memcmp(shm_ptr, "can u see me now", strlen("can u see me now") + 1);
        printf("child just wrote: %s", (char*) shm_ptr);
        exit(0);
    } else {
        printf("Parent process starting\n");
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        printf("Child process exited with status: %d\n", status);
        printf("Message from the child process: %s\n", (char *)shm_ptr);

        // Unmap and close the shared memory object
        if (munmap(shm_ptr, SHM_SIZE) == -1) {
            exit(1);
        }
        if (shm_unlink(SHM_NAME) == -1) {
            exit(1);
        }
    }

    return 0;
}