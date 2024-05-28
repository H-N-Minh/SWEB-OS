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