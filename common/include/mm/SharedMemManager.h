
#pragma once
#include "types.h"
#include "umultimap.h"
#include "Mutex.h"


#define PROT_NONE     0x00000000  // 00..00
#define PROT_READ     0x00000001  // ..0001
#define PROT_WRITE    0x00000002  // ..0010
#define PROT_EXEC     0x00000004  // ..0100

#define MAP_PRIVATE   0x20000000  // 0010..
#define MAP_SHARED    0x40000000  // 0100..
#define MAP_ANONYMOUS 0x80000000  // 1000..

#define MAP_FAILED	((void *) -1)

typedef size_t vpn_t;

// struct to store parameters for mmap
typedef struct mmap_params {
  void* start;
  size_t length;
  int prot;
  uint32 flags;
  int fd;
  ssize_t offset;
}mmap_params_t;


// struct to store all information of each shared memory entry within a process
class SharedMemEntry
{
public:
  vpn_t start_;
  vpn_t end_;
  int prot_;
  int flags_;
  int fd_;
  ssize_t offset_;

  SharedMemEntry(vpn_t start, vpn_t end, int prot, int flags, int fd, ssize_t offset);

  /**
   * check if the given vpn is within this shared memory block
  */
  bool isInBlockRange(vpn_t vpn);

  /**
   * get the size of the mem block (in pages)
  */
  size_t getSize();
};


class SharedMemManager
{
private:
  ustl::map<vpn_t, SharedMemEntry*> shared_map_;
  vpn_t last_free_vpn_;

public:
  Mutex shared_mem_lock_;

  
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

  /**
   * add an entry to the shared memory map
   * @return the starting address of the shared memory region
   * @return MAP_FAILED if the shared memory region is full
  */
  void* addEntry(void* addr, size_t length, int prot, int flags, int fd, ssize_t offset);

  /**
   * check if the given address is within any shared memory block
  */
  bool isAddressValid(vpn_t vpn);
};