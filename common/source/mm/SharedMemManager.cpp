#include "SharedMemManager.h"
#include "assert.h"
#include "debug.h"


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
    return nullptr;
}