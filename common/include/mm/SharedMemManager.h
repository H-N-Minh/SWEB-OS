
#pragma once
#include "types.h"

typedef struct mmap_params {
  void* start;
  size_t length;
  int prot;
  int flags;
  int fd;
  ssize_t offset;
}mmap_params_t;


class SharedMemManager
{
public:
  SharedMemManager();
  // TODOMINH: add copy constructor so it works with fork() (maybe also add cpy ctor for UserSpaceMemManager)
  ~SharedMemManager();
  void* mmap(mmap_params_t* params);
  int munmap(void* addr, size_t length);
  void* getAddr();
  size_t getLength();
  int getProt();
  int getFlags();
  int getFd();
  ssize_t getOffset();
};