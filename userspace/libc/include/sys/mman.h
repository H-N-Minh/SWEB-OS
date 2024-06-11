#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// these should in sync with UserSpaceMemoryManager.h
#define __PAGE_SIZE__ 4096
#define __USER_BREAK__ 0x0000800000000000ULL
#define __MAX_HEAP_ADDRESS__ (__USER_BREAK__ / 4)

#define __MIN_SHARED_MEM_ADDRESS__ (__MAX_HEAP_ADDRESS__ + (1000 * __PAGE_SIZE__))
#define __MAX_SHARED_MEM_ADDRESS__ (__USER_BREAK__ / 2)



#define PROT_NONE     0x00000000  // 00..00
#define PROT_READ     0x00000001  // ..0001
#define PROT_WRITE    0x00000002  // ..0010
#define PROT_EXEC     0x00000004  // ..0100

#define MAP_PRIVATE   0x20000000  // 0010..
#define MAP_SHARED    0x40000000  // 0100..
#define MAP_ANONYMOUS 0x80000000  // 1000..

#define MAP_FAILED	((void *) -1)

extern void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);

extern int munmap(void* start, size_t length);

extern int shm_open(const char* name, int oflag, mode_t mode);

extern int shm_unlink(const char* name);

extern int mprotect(void *addr, size_t len, int prot);

#ifdef __cplusplus
}
#endif


