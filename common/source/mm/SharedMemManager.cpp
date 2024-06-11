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


/////////////////////// SharedMemEntry ///////////////////////

SharedMemEntry::SharedMemEntry(vpn_t start, vpn_t end, int prot, int flags, int fd, ssize_t offset, bool shared)
    : start_(start),end_(end), prot_(prot), flags_(flags), fd_(fd), offset_(offset), shared_(shared)
{
}

bool SharedMemEntry::isInBlockRange(vpn_t vpn)
{
    return vpn >= start_ && vpn <= end_;
}

size_t SharedMemEntry::getSize()
{
    return end_ - start_ + 1;
}

/////////////////////// SharedMemManager ///////////////////////


SharedMemManager::SharedMemManager()
    : shared_mem_lock_("shared_mem_lock_")
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

    shared_mem_lock_.acquire();

    if ((flags == (MAP_ANONYMOUS | MAP_PRIVATE)) && fd == -1)
    {
        retval = addEntry(start, length, prot, flags, fd, offset, false);
    }
    
    else if ((flags == (MAP_ANONYMOUS | MAP_SHARED)) && fd == -1)
    {
        retval = addEntry(start, length, prot, flags, fd, offset, true);
    }



    if (retval == MAP_FAILED)
    {
        debug(ERROR_DEBUG, "SharedMemManager::mmap: failed\n");
    }

    shared_mem_lock_.release();
    
    return retval;
}

void* SharedMemManager::addEntry(void* addr, size_t length, int prot, int flags, int fd, ssize_t offset, bool shared)
{
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::addEntry: shared_mem_lock_ not held\n");

    debug(MMAP, "SharedMemManager::addEntry: adding entry to shared_map_ addr: %p, length: %zu, prot: %d, flags: %d, fd: %d, offset: %ld\n",addr, length, prot, flags, fd, offset);
    if (last_free_vpn_ > MAX_SHARED_MEM_VPN)
    {
        debug(ERROR_DEBUG, "SharedMemManager::addEntry: failed, out of shared memory\n");
        return MAP_FAILED;
    }

    int size = length / PAGE_SIZE;
    if (length % PAGE_SIZE != 0)
    {
        size++;
    }
    vpn_t start = last_free_vpn_;
    vpn_t end = start + size - 1;

    assert(start >= MIN_SHARED_MEM_VPN && end >= MIN_SHARED_MEM_VPN && "SharedMemManager::addEntry: start or end is out of range\n");
    assert(start <= MAX_SHARED_MEM_VPN && end <= MAX_SHARED_MEM_VPN && "SharedMemManager::addEntry: start or end is out of range\n");
    if (last_free_vpn_ > MAX_SHARED_MEM_VPN)
    {
        debug(ERROR_DEBUG, "SharedMemManager::addEntry: failed, size required is bigger than the available free pages \n");
        return MAP_FAILED;
    }
    
    // actually adding entry
    SharedMemEntry* entry = new SharedMemEntry(start, end, prot, flags, fd, offset, shared);
    shared_map_.push_back(entry);

    void* start_addr = (void*) (start * PAGE_SIZE);
    last_free_vpn_ += size;
    debug(MMAP, "SharedMemManager::addEntry: added shared block (start: %p, size: %zu pages) to process %d\n", start_addr, entry->getSize(), ((UserThread*) currentThread)->process_->pid_);
    return start_addr;
}


bool SharedMemManager::isAddressValid(size_t address)
{
    assert(address && "SharedMemManager::isAddressValid: invalid address\n");
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::isAddressValid: shared_mem_lock_ not held\n");

    vpn_t vpn = address / PAGE_SIZE;
    if (address >= MIN_SHARED_MEM_ADDRESS && address < MAX_SHARED_MEM_ADDRESS)
    {
        for (auto it : shared_map_)
        {
            if (it->isInBlockRange(vpn))
            {
                return true;
            }
        }
    }
    return false;
}

void SharedMemManager::handleSharedPF(ustl::vector<uint32>& preallocated_pages, size_t address)
{
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::handleSharedPF: shared_mem_lock_ not held\n");
    assert(currentThread->loader_->arch_memory_.archmemory_lock_.isHeldBy(currentThread) && "SharedMemManager::handleSharedPF: archmemory_lock_ not held\n");

    size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);
    size_t vpn = address / PAGE_SIZE;
    bool rv = currentThread->loader_->arch_memory_.mapPage(vpn, ppn, 1, preallocated_pages);
    assert(rv == true);

    debug(MMAP, "SharedMemManager::handleSharedPF: setting protection for the new page\n");
    // Set the read bit to false
    int read = 0;
    int write = 0;
    int execute = 0;

    SharedMemEntry* entry = getSharedMemEntry(address);
    if (entry->prot_ == PROT_READ)
    {
        read = 1;
    }
    else if (entry->prot_ == PROT_WRITE)
    {
        write = 1;
    }
    else if (entry->prot_ == PROT_EXEC)
    {
        execute = 1;
    }
    else if (entry->prot_ == (PROT_READ | PROT_WRITE))
    {
        read = 1;
        write = 1;
    }
    else if (entry->prot_ == (PROT_READ | PROT_EXEC))
    {
        read = 1;
        execute = 1;
    }
    else if (entry->prot_ == (PROT_WRITE | PROT_EXEC))
    {
        write = 1;
        execute = 1;
    }
    else if (entry->prot_ == (PROT_READ | PROT_WRITE | PROT_EXEC))
    {
        read = 1;
        write = 1;
        execute = 1;
    }
    else if (entry->prot_ == PROT_NONE)
    {
        debug(MMAP, "SharedMemManager::handleSharedPF: PROT_NONE => no protection\n");
    }
    else
    {
        assert(false && "SharedMemManager::handleSharedPF: invalid protection bits\n");
    }

    currentThread->loader_->arch_memory_.setProtectionBits(vpn, read, write, execute);
}


SharedMemEntry* SharedMemManager::getSharedMemEntry(size_t address)
{
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::getSharedMemEntry: shared_mem_lock_ not held\n");

    vpn_t vpn = address / PAGE_SIZE;
    for (auto it : shared_map_)
    {
        if (it->isInBlockRange(vpn))
        {
            return it;
        }
    }
    return nullptr;
}


int SharedMemManager::munmap(void* start, size_t length)
{
    debug(MMAP, "SharedMemManager::munmap: start: %p, length: %zu\n", start, length);
    Mutex* ipt_lock = &IPTManager::instance()->IPT_lock_;
    Mutex* archmem_lock = &currentThread->loader_->arch_memory_.archmemory_lock_;

    shared_mem_lock_.acquire();
    ipt_lock->acquire();
    archmem_lock->acquire();

    
    // check if the munmap is valid, or already unmapped
    ustl::vector<ustl::pair<vpn_t, SharedMemEntry*>> relevant_pages;
    findRevelantPages(relevant_pages, (size_t) start, length);
    if (relevant_pages.empty())
    {
        debug(ERROR_DEBUG, "SharedMemManager::munmap: the given range includes pages that are not shared mem\n");
        archmem_lock->release();
        ipt_lock->release();
        shared_mem_lock_.release();
        return -1;
    }

    // unmapping each page
    for (auto it : relevant_pages)
    {
        unmapOnePage(it.first, it.second);
    }


    archmem_lock->release();
    ipt_lock->release();
    shared_mem_lock_.release();

    return 0;
}

void SharedMemManager::findRevelantPages(ustl::vector<ustl::pair<vpn_t, SharedMemEntry*>> &relevant_pages, size_t start, size_t length)
{
    debug(MMAP, "SharedMemManager::findRevelantPages: start: %zu, length: %zu. Looping through all shared mem and find relevant pages to unmap\n", start, length);
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::findRevelantPages: shared_mem_lock_ not held\n");
    assert(start >= MIN_SHARED_MEM_ADDRESS && start < MAX_SHARED_MEM_ADDRESS && "SharedMemManager::findRevelantPages: start out of range\n");
    assert(length > 0 && "SharedMemManager::findRevelantPages: invalid length\n");
    assert(relevant_pages.empty() && "SharedMemManager::findRevelantPages: relevant_pages not empty\n");

    vpn_t start_vpn = start / PAGE_SIZE;
    vpn_t end_vpn = (start + length - 1) / PAGE_SIZE;

    for (auto it : shared_map_)
    {
        if (it->isInBlockRange(start_vpn))
        {
            if (it->isInBlockRange(end_vpn))
            {
                // both start and end vpn is in the same mem block
                for (vpn_t vpn = start_vpn; vpn <= end_vpn; vpn++)
                {
                    relevant_pages.push_back(ustl::make_pair(vpn, it));
                }
                return;
            }
            else
            {
                // start vpn is in the mem block, but end vpn is not
                for (vpn_t vpn = start_vpn; vpn <= it->end_; vpn++)
                {
                    relevant_pages.push_back(ustl::make_pair(vpn, it));
                }
                start_vpn = it->end_ + 1;
                assert(start_vpn <= end_vpn && "SharedMemManager::findRevelantPages: start_vpn > end_vpn\n");
                assert(start_vpn >= MIN_SHARED_MEM_VPN && start_vpn <= MAX_SHARED_MEM_VPN && "SharedMemManager::findRevelantPages: start_vpn becomes out of range\n");
            }
        }
        else
        {
            if (it->isInBlockRange(end_vpn))
            {
                // start vpn is not in the mem block, but end vpn is
                for (vpn_t vpn = it->start_; vpn <= end_vpn; vpn++)
                {
                    relevant_pages.push_back(ustl::make_pair(vpn, it));
                }
                end_vpn = it->start_ - 1;
                assert(start_vpn <= end_vpn && "SharedMemManager::findRevelantPages: start_vpn > end_vpn\n");
                assert(end_vpn >= MIN_SHARED_MEM_VPN && end_vpn <= MAX_SHARED_MEM_VPN && "SharedMemManager::findRevelantPages: end_vpn becomes out of range\n");
            }
        
        }
    }
    debug(ERROR_DEBUG, "SharedMemManager::findRevelantPages: this shouldnt be reached, the given range is invalid\n");
    relevant_pages.clear();
}


void SharedMemManager::unmapOnePage(vpn_t vpn, SharedMemEntry* sm_entry)
{
    debug(MMAP, "SharedMemManager::unmapOnePage: vpn: %zu\n", vpn);
    ArchMemory* arch_memory = &((UserThread*) currentThread)->loader_->arch_memory_;
    assert(sm_entry && "SharedMemManager::unmapOnePage: invalid sm_entry\n");
    auto it = ustl::find(shared_map_.begin(), shared_map_.end(), sm_entry);
    assert(it != shared_map_.end() && "SharedMemManager::unmapOnePage: sm_entry not found in shared_map_\n");
    if (sm_entry->start_ > vpn || sm_entry->end_ < vpn)
    {
        assert(false && "SharedMemManager::unmapOnePage: vpn not in range of sm_entry\n");
    }
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::unmapOnePage: shared_mem_lock_ not held\n");
    assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread && "SharedMemManager::unmapOnePage: IPT need to be locked");
    assert(arch_memory->archmemory_lock_.heldBy() == currentThread && "SharedMemManager::unmapOnePage: archmemory_lock_ not held\n");
    assert(sm_entry->getSize() > 0 && "SharedMemManager::unmapOnePage: sm_entry size is 0\n");
    
    // split the memory segment if necessary
    if (sm_entry->getSize() == 1)
    {
        shared_map_.erase(it);
        delete sm_entry;
    }
    else  // size > 1
    {
        if (vpn == sm_entry->start_)
        {
            sm_entry->start_++;
        }
        else if (vpn == sm_entry->end_)
        {
            sm_entry->end_--;
        }
        else
        {
            shared_map_.push_back(new SharedMemEntry(sm_entry->start_, vpn - 1, sm_entry->prot_, sm_entry->flags_, 
                                                    sm_entry->fd_, sm_entry->offset_, sm_entry->shared_));
            shared_map_.push_back(new SharedMemEntry(vpn + 1, sm_entry->end_, sm_entry->prot_, sm_entry->flags_, 
                                                    sm_entry->fd_, sm_entry->offset_, sm_entry->shared_));
            shared_map_.erase(it);
            delete sm_entry;
        }
    }
    
    // upmap if necessary
    if (arch_memory->checkAddressValid(vpn * PAGE_SIZE))
    {
        debug(MMAP, "SharedMemManager::unmapOnePage: unmapping page vpn %zu\n", vpn);
        arch_memory->unmapPage(vpn);
    }
}


void SharedMemManager::unmapAllPages()
{
    debug(MMAP, "SharedMemManager::unmapAllPages unmaping all shared mem pages\n");
    ArchMemory* arch_memory = &((UserThread*) currentThread)->loader_->arch_memory_;

    shared_mem_lock_.acquire();
    IPTManager::instance()->IPT_lock_.acquire();
    arch_memory->archmemory_lock_.acquire();

    for (auto it : shared_map_)
    {
        for (vpn_t vpn = it->start_; vpn <= it->end_; vpn++)
        {
            if (arch_memory->checkAddressValid(vpn * PAGE_SIZE))
            {
                // debug(MMAP, "SharedMemManager::unmapAllPages: unmapping page vpn %zu\n", vpn);
                arch_memory->unmapPage(vpn);
            }
        }
    }
    arch_memory->archmemory_lock_.release();
    IPTManager::instance()->IPT_lock_.release();
    shared_mem_lock_.release();
    debug(MMAP, "SharedMemManager::unmapAllPages: done\n");
}

