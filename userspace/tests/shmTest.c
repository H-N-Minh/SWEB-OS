#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#include <sys/mman.h> // For mmap, shm_open


#define SHM_NAME "/shmTest"
#define SHM_SIZE 1024

int main() {
    int shm_fd;
    // void *shm_ptr = (void *)0x60000000;

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
			printf("error");
    	return -1;
    }

    const char *message = "Hello, Shared Memory!";
    // memcpy(shm_ptr, message, strlen(message) + 1);
    //
    // printf("Read from shared memory: %s\n", (char *)shm_ptr);



    return 0;
}
