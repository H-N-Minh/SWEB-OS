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
	printf("Testing shm_normal: This test should be ran twice back to back in terminal\n");

	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	if (shm_fd == -1)
	{
		assert(0 && "Should not return -1");
	}
	shm_unlink(SHM_NAME);

	return 0;
}
