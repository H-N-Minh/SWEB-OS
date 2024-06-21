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

	printf("Testing shm0: unlink non existed object and create object without O_CREAT flag\n");
	int unlink = shm_unlink(SHM_NAME);
	if(unlink != -1)
	{
		assert(0 && "Should return -1");
	}

	int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
	if(shm_fd != -1)
	{
		assert(0 && "Should return -1");
	}
	return 0;
}
