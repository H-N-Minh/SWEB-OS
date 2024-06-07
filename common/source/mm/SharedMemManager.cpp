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



SharedMemManager::SharedMemManager()
{
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
    int flags = params->flags;
    int fd  = params->fd;
    ssize_t offset = params->offset;
    debug(MINH, "SharedMemManager::mmap: start: %p, length: %zu, prot: %d, flags: %d, fd: %d, offset: %ld\n",start, length, prot, flags, fd, offset);
    

    ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAllocatePages(4);  // mapPage needs 3 and ppn needs 1

    size_t ppn = PageManager::instance()->getPreAllocatedPage(preallocated_pages);

    ArchMemory* arch = &((UserThread*) currentThread)->loader_->arch_memory_;
    IPTManager::instance()->IPT_lock_.acquire();
    arch->archmemory_lock_.acquire();
    shared_map_.insert(ustl::make_pair(fd, ppn));

    // map the page to the given address
    size_t vpn = MAX_HEAP_SIZE * 3 / PAGE_SIZE;

    bool stat = ((UserThread*) currentThread)->loader_->arch_memory_.mapPage(vpn, ppn, 1, preallocated_pages);
    assert(stat && "SharedMemManager::mmap: mapPage failed\n");

    // Read the data from the file into the page with the given ppn
    char buffer[100];
    size_t bytes_read = Syscall::read(fd, (pointer) buffer, 20);
    debug(MINH, "SharedMemManager::mmap: bytesRead: %zu\n", bytes_read);
    buffer[bytes_read] = '\0';
    // if (bytesRead == -1) {
    //     // Handle read error
    //     debug(ERROR_DEBUG, "SharedMemManager::mmap: read failed\n");
    //     return nullptr;
    // }
    if (bytes_read < (size_t) length) {
        // Handle incomplete read
        debug(ERROR_DEBUG, "SharedMemManager::mmap: incomplete read\n");
        return nullptr;
    }
    
    arch->archmemory_lock_.release();
    IPTManager::instance()->IPT_lock_.release();
    return (void*) (MAX_HEAP_SIZE * 3);
}

