#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include "wait.h"

#define SHM_NAME "/shmTest2"
#define SHM_SIZE 1024

int main() {
	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
	int shm_fd1 = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

	if (shm_fd == -1 || shm_fd1 == -1) {
		printf("shm_open failed\n");
		return 1;
	}

	printf("%d \n", shm_fd);
	printf("%d \n", shm_fd1);

	void *addr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (addr == MAP_FAILED) {
		printf("mmap failed\n");
		shm_unlink(SHM_NAME);
		return 1;
	}

	char *data = "Hello from shmTest2!";
	memcpy(addr, data, strlen(data) + 1);

	printf("Data in shared memory: %s\n", (char *)addr);

	munmap(addr, SHM_SIZE);
	close(shm_fd);
	close(shm_fd1);
	shm_unlink(SHM_NAME);

	return 0;
}