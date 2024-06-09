#include "SharedMemManager.h"
#include "assert.h"
#include "debug.h"
#include "PageManager.h"
#include "UserSpaceMemoryManager.h"
#include "paging-definitions.h"
#include "UserThread.h"
#include "UserProcess.h"
#include "IPTManager.h"
#include "Syscall.h"

SharedMemEntry::SharedMemEntry(size_t size, int prot, int flags, int fd, ssize_t offset)
    : size_(size), prot_(prot), flags_(flags), fd_(fd), offset_(offset)
{
}



SharedMemManager::SharedMemManager()
{
    last_free_vpn_ = (vpn_t) MIN_SHARED_MEM_VPN;
}

SharedMemManager::~SharedMemManager()
{
}

void* SharedMemManager::mmap(mmap_params_t* params)
{
    assert(params && "SharedMemManager::mmap: params is null\n");
    void* start = params->start;
    size_t length = params->length;
    int prot = params->prot;
    uint32 flags = params->flags;
    int fd  = params->fd;
    ssize_t offset = params->offset;
    debug(MMAP, "SharedMemManager::mmap: start: %p, length: %zu, prot: %d, flags: %d, fd: %d, offset: %ld\n",start, length, prot, flags, fd, offset);
    
    void* retval = MAP_FAILED;

    if ((flags == (MAP_ANONYMOUS | MAP_PRIVATE)) && fd == -1)
    {
        retval = addEntry(start, length, prot, flags, fd, offset);
    }
    

    // ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAlocatePages(4);  // mapPage needs 3 and ppn needs 1

    // size_t ppn =  PageManager::instance()->getPreAlocatedPage(preallocated_pages);

    // ArchMemory* arch = &((UserThread*) currentThread)->loader_->arch_memory_;
    // IPTManager::instance()->IPT_lock_.acquire();
    // arch->archmemory_lock_.acquire();
    // size_t vpn = MAX_HEAP_ADDRESS * 3 / PAGE_SIZE;
    // shared_map_.insert(ustl::make_pair(fd, ppn));

    // map the page to the given address

    // bool stat = ((UserThread*) currentThread)->loader_->arch_memory_.mapPage(vpn, ppn, 1, preallocated_pages);
    // assert(stat && "SharedMemManager::mmap: mapPage failed\n");

    // // Read the data from the file into the page with the given ppn
    // char buffer[100];
    // size_t bytes_read = Syscall::read(fd, (pointer) buffer, 20);
    // debug(MINH, "SharedMemManager::mmap: bytesRead: %zu\n", bytes_read);
    // buffer[bytes_read] = '\0';
    // if (bytesRead == -1) {
    //     // Handle read error
    //     debug(ERROR_DEBUG, "SharedMemManager::mmap: read failed\n");
    //     return nullptr;
    // }
    // if (bytes_read < (size_t) length) {
    //     // Handle incomplete read
    //     debug(ERROR_DEBUG, "SharedMemManager::mmap: incomplete read\n");
    //     return nullptr;
    // }
    
    // arch->archmemory_lock_.release();
    // IPTManager::instance()->IPT_lock_.release();
    if (retval == MAP_FAILED)
    {
        debug(MMAP, "SharedMemManager::mmap: failed\n");
    }
    
    return retval;
}

void* SharedMemManager::addEntry(void* addr, size_t length, int prot, int flags, int fd, ssize_t offset)
{
    debug(MMAP, "SharedMemManager::addEntry: adding entry to shared_map_ addr: %p, length: %zu, prot: %d, flags: %d, fd: %d, offset: %ld\n",addr, length, prot, flags, fd, offset);
    if (last_free_vpn_ > MAX_SHARED_MEM_VPN)
    {
        debug(MMAP, "SharedMemManager::addEntry: failed, out of shared memory\n");
        return MAP_FAILED;
    }


    int size = length / PAGE_SIZE;
    if (length % PAGE_SIZE != 0)
    {
        size++;
    }
    
    SharedMemEntry* entry = new SharedMemEntry(size, prot, flags, fd, offset);
    shared_map_[last_free_vpn_] = entry;

    size_t start_addr = last_free_vpn_ * PAGE_SIZE;
    last_free_vpn_ += size;
    debug(MMAP, "SharedMemManager::addEntry: added shared block (start: %p) to process %d\n", (void*) start_addr, ((UserThread*) currentThread)->process_->pid_);
    return (void*) start_addr;
}

void* SharedMemManager::fakeMalloc(void* start, size_t length, int prot)
{    
    debug(MMAP, "SharedMemManager::fakeMalloc: start: %p, length: %zu, prot: %d\n",start, length, prot);


    // // map the page to the given address
    // size_t vpn = MAX_HEAP_ADDRESS * 3 / PAGE_SIZE;
    // shared_map_.insert(ustl::make_pair(fd, vpn));

    return (void*) (MAX_HEAP_ADDRESS * 3);
}

