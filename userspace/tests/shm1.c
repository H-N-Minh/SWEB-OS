#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "assert.h"

#include <sys/mman.h> // For mmap, shm_open


#define SHM_NAME "/shmTest"
#define SHM_SIZE 1024

int main()
{
	printf("Testing order should be: \n");
	printf("shm0 -> shm1 -> shm2 \n\n");

	printf("Testing shm1: create 2 objects with thew same name\n");

	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	int shm_fd1 = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd != shm_fd1 || shm_fd == -1 || shm_fd1 == -1)
  {
  	assert(0 && "Should not return -1 and 2 fds must be the same");
  }

	printf("fd %d \n", shm_fd);
	return 0;
}
