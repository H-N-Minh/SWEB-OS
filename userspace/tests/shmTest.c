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
	int shm_fd;
	// void *shm_ptr = (void *)0x60000000;

	shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	int shm_fd1 = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1)
  {
		printf("error");
    return -1;
  }

	printf("%d \n", shm_fd);
	printf("%d \n", shm_fd1);

	shm_unlink(SHM_NAME);

	return 0;
}
