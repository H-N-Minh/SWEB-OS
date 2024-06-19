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

int __VALID_FLAGS__[] = {MAP_PRIVATE, MAP_SHARED, MAP_PRIVATE | MAP_ANONYMOUS, MAP_SHARED | MAP_ANONYMOUS, 
                         MAP_ANONYMOUS | MAP_PRIVATE, MAP_ANONYMOUS | MAP_SHARED};


#include "../../../common/include/kernel/syscall-definitions.h"
#include "sys/syscall.h"

/**
 * function stub
 * posix compatible signature - do not change the signature!
 * @param start the starting address of the memory region to be mapped. Null for letting the kernel choose the address.
 */
void* mmap(void* start, size_t length, int prot, int flags, int fd,
           off_t offset)
{
  int valid_flags = sizeof(__VALID_FLAGS__) / sizeof(__VALID_FLAGS__[0]);
  int flag_exists = 0;
  for (int i = 0; i < valid_flags; i++) 
  {
    if (flags == __VALID_FLAGS__[i])
    {
      flag_exists = 1;
      break;
    }
  }

  if (!flag_exists)
  {
    return MAP_FAILED;
  }

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
  // start must be multiple of page size, length must be greater than 0
  if ((size_t) start % __PAGE_SIZE__ != 0 || length == 0)
  {
    return -1;
  }

  // round up length to find the end address (multiple of pagesize)
  int pages = length / __PAGE_SIZE__;
  if (length % __PAGE_SIZE__ != 0)
  {
    pages++;
  }
  size_t end = (size_t) start + pages * __PAGE_SIZE__;

  // Check if range is within shared memory range
  if (end > __MAX_SHARED_MEM_ADDRESS__ || (size_t) start < __MIN_SHARED_MEM_ADDRESS__)
  {
    return -1;
  }
  
  return __syscall(sc_munmap, (size_t) start, length, 0, 0, 0);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int shm_open(const char* name, int oflag, mode_t mode)
{
	return __syscall(sc_shm_open, (long)name, oflag, mode, 0x00, 0x00);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int shm_unlink(const char* name)
{
	return __syscall(sc_shm_unlink, (long)name, 0x00, 0x00, 0x00, 0x00);
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int mprotect(void *addr, size_t len, int prot)
{
  return -1;
}

