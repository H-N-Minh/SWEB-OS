#include "nonstd.h"
#include "sys/syscall.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "stdlib.h"
#include "assert.h"

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
  if (pra != 0 && pra != 1 && pra!= 2)
  {
    assert(0);
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

int getSwappingStats(int* disk_writes, int* disk_reads, int* discard_unchanged_page, int* reuse_same_disk_location)
{
  return __syscall(sc_swappingStats, (size_t) disk_writes, (size_t) disk_reads, (size_t)discard_unchanged_page, (size_t)reuse_same_disk_location, 0x0);
}