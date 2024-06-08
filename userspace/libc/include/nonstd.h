#pragma once

#include "../../../common/include/kernel/syscall-definitions.h"

#include "stdarg.h"
#include "types.h"
#include "sys/syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __RANDOM_PRA__ 0
#define __NFU_PRA__ 1
#define __SECOND_CHANCE_PRA__ 2

/**
 * Creates a new process.
 *
 * @param path the path to the binary to open
 * @param sleep whether the calling process should sleep until the other process terminated
 * @return -1 if the path did not lead to an executable, 0 if the process was executed successfully
 *
 */ 
extern int createprocess(const char* path, int sleep);

extern int getIPTInfos();
extern int assertIPT();
extern int setPRA(int pra);
extern int getPRAstats(int* hit_count, int* miss_count);
extern int checkRandomPRA();

#ifdef __cplusplus
}
#endif



