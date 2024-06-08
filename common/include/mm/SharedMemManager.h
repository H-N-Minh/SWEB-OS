
#pragma once
#include "types.h"
#include "umultimap.h"


#define PROT_NONE     0x00000000  // 00..00
#define PROT_READ     0x00000001  // ..0001
#define PROT_WRITE    0x00000002  // ..0010
#define PROT_EXEC     0x00000004  // ..0100

#define MAP_PRIVATE   0x20000000  // 0010..
#define MAP_SHARED    0x40000000  // 0100..
#define MAP_ANONYMOUS 0x80000000  // 1000..

#define MAP_FAILED	((void *) -1)


typedef struct mmap_params {
  void* start;
  size_t length;
  int prot;
  int flags;
  int fd;
  ssize_t offset;
}mmap_params_t;

class SharedMemEntry
{
public:
  int length_;    // in pages
  int prot_;
  int flags_;
  int fd_;
  ssize_t offset_;

  SharedMemEntry(void* addr, size_t length, int prot, int flags, int fd, ssize_t offset)
  {
    addr_ = addr;
    length_ = length;
    prot_ = prot;
    flags_ = flags;
    fd_ = fd;
    offset_ = offset;
  }
};

class SharedMemManager
{
public:
  ustl::multimap<int, size_t> shared_map_;  // <fd, vpn>

  
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

  void* fakeMalloc(void* start, size_t length, int prot);

};