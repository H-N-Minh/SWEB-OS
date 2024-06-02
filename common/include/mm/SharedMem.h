
#pragma once
#include "types.h"

typedef struct mmap_params {
  void* start;
  size_t length;
  int prot;
  int flags;
  int fd;
  off_t offset;
}mmap_params_t;


