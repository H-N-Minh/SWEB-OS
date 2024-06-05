#include "sys/mman.h"
#include "sys/syscall.h"
#include "types.h"
#include "../../../common/include/kernel/syscall-definitions.h"

typedef struct mmap_params {
  void* start;
  size_t length;
  int prot;
  int flags;
  int fd;
  off_t offset;
}mmap_params_t;


/**
 * function stub
 * posix compatible signature - do not change the signature!
 * @param start the starting address of the memory region to be mapped. Null for letting the kernel choose the address.
 */
void* mmap(void* start, size_t length, int prot, int flags, int fd,
           off_t offset)
{
  size_t retval;
  mmap_params_t params = {start, length, prot, flags, fd, offset};
  __syscall(sc_mmap, (size_t) &params, (size_t) &retval, 0, 0, 0);
  return (void*) retval;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int munmap(void* start, size_t length)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int shm_open(const char* name, int oflag, mode_t mode)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int shm_unlink(const char* name)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int mprotect(void *addr, size_t len, int prot)
{
  return -1;
}

