#include "unistd.h"


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int brk(void *end_data_segment)
{
  return -1;
}

/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
void* sbrk(intptr_t increment)
{
  void* allocated_space = 0;
  int retval = __syscall(sc_sbrk, (size_t) &increment, (size_t) &allocated_space, 0x0, 0x0, 0x0);
  if (retval != 0)
  {
    return (void*) -1;
  }
  else
  {
    return allocated_space;
  }
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
unsigned int sleep(unsigned int seconds)
{
  return  __syscall(sc_sleep, (size_t)seconds, 0x0, 0x0, 0x0, 0x0); //not implemented
}


/**
 * function stub
 * posix compatible signature - do not change the signature!
 */
int ftruncate(int fildes, off_t length)
{
    return -1;
}
