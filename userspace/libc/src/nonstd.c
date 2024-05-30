#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"

int createprocess(const char* path, int sleep)
{
  return __syscall(sc_createprocess, (long) path, sleep, 0x00, 0x00, 0x00);
}

extern int main();

void _start()
{
  exit(main());
}


int getIPTInfos()
{
  return __syscall(sc_getIPTInfos, 0x00, 0x00, 0x00, 0x00, 0x00);
}

int assertIPT()
{
  return __syscall(sc_assertIPT, 0x0, 0x0, 0x0, 0x0, 0x0);
}

int setPRA(int pra)
{
  if (pra != 0 && pra != 1)
  {
    return -1;
  }
  return __syscall(sc_setPRA, (size_t) pra, 0x0, 0x0, 0x0, 0x0);
  
}

int getPRAstats(int* hit_count, int* miss_count)
{
  return __syscall(sc_getPRAstats, (size_t) hit_count, (size_t) miss_count, 0x0, 0x0, 0x0);
}

int checkRandomPRA()
{
  return __syscall(sc_checkRandomPRA, 0x0, 0x0, 0x0, 0x0, 0x0);
}