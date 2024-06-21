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

	printf("Testing shm2\n");
	printf("create object that already exists but O_EXCL flag is on\n\n");
	int shm_fd = shm_open(SHM_NAME, O_EXCL, 0666);

	if(shm_fd != -1)
	{
		assert(0 && "Should return -1");
	}
	printf("unlink then create object\n");
	shm_unlink(SHM_NAME);
	shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	printf("fd %d \n", shm_fd);
	shm_unlink(SHM_NAME);
	return 0;
}