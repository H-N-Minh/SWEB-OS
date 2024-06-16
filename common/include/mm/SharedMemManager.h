#pragma once
#include "types.h"
#include "umultimap.h"
#include "Mutex.h"
#include "ustring.h"
#include "FileDescriptor.h"
// #include "FileDescriptorList.h"


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

extern FileDescriptorList global_fd_list;

// // struct to store all information of each shared memory entry within a process
// class SharedMemEntry
// {
// public:
//   vpn_t start_;
//   vpn_t end_;
//   int prot_;
//   int flags_;
//   int fd_;
//   ssize_t offset_;
//
//   SharedMemEntry(vpn_t start, vpn_t end, int prot, int flags, int fd, ssize_t offset);
//
//   /**
//    * check if the given vpn is within this shared memory block
//   */
//   bool isInBlockRange(vpn_t vpn);
//
//   /**
//    * get the size of the mem block (in pages)
//   */
//   size_t getSize();
// };

class SharedMemObject {
public:
    typedef size_t vpn_t;

    ustl::string name_;
    vpn_t start_;
    vpn_t end_;
    int prot_;
    int flags_;
    int fd_;
    ssize_t offset_;

    SharedMemObject(const ustl::string& name, vpn_t start, vpn_t end, int prot, int flags, int fd, ssize_t offset)
        : name_(name), start_(start), end_(end), prot_(prot), flags_(flags), fd_(fd), offset_(offset)
    {
     global_fd_ = new FileDescriptor(nullptr, FileDescriptor::FileType::SHARED_MEMORY);
    }

    bool isInBlockRange(vpn_t vpn) const {
        return vpn >= start_ && vpn <= end_;
    }

    size_t getSize() const {
        return end_ - start_ + 1;
    }

    FileDescriptor* global_fd_;
	FileDescriptor* getGlobalFileDescriptor() const;

};


class SharedMemManager
{
private:
  ustl::vector<SharedMemObject*> shared_map_;
  vpn_t last_free_vpn_;

  ustl::map<ustl::string, SharedMemObject*> shm_objects_;

public:
  Mutex shared_mem_lock_;

  
  SharedMemManager();
  // TODOMINH: add copy constructor so it works with fork() (maybe also add cpy ctor for UserSpaceMemManager)
  ~SharedMemManager();


  void* mmap(mmap_params_t* params);
  int munmap(void* addr, size_t length);

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

  /**
   * handle the page fault that is related to shared memory. also Setting the protections for the page
  */
  void handleSharedPF(ustl::vector<uint32>& preallocated_pages, size_t address);

  /**
   * get the shared memory entry that contains the given address
  */
  SharedMemObject* getSharedMemEntry(size_t address);

  /**
   * unmap one page from the shared memory. The vpn should already be a valid page that can be remove
  */
  void unmapOnePage(vpn_t vpn, SharedMemObject* sm_entry);

  /**
   * find all relevant pages that are within the given range and fill up the vector
   * @param relevant_pages: an empty vector to be filled up
   * @param start: the starting address of the range
   * @param length: the length of the range (in bytes)
   * @return: the vector filled up with all relevant pages. Vector is empty if error
  */
  void findRevelantPages(ustl::vector<ustl::pair<vpn_t, SharedMemObject*>> &relevant_pages, size_t start, size_t length);

  /**
   * unmap all active shared mem pages. Used for process termination
  */
  void unmapAllPages();

  int shm_open(char* name, size_t oflag, mode_t mode);
  int shm_unlink(char* name);

};

// // struct to store parameters for shared memory object
// class SharedMemObject : public SharedMemEntry {
// public:
//  ustl::string name_;
//
//  SharedMemObject(const ustl::string& name, vpn_t start, vpn_t end, int prot, int flags, int fd, ssize_t offset)
//    : SharedMemEntry(start, end, prot, flags, fd, offset), name_(name) {}
// };


