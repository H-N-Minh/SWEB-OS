#include "SharedMemManager.h"
#include "assert.h"
#include "debug.h"
#include "PageManager.h"
#include "UserSpaceMemoryManager.h"
#include "paging-definitions.h"
#include "UserThread.h"
#include "UserProcess.h"
#include "VfsSyscall.h"
#include "Syscall.h"
#include "File.h"
#include "syscall-definitions.h"

/////////////////////// SharedMemEntry ///////////////////////

SharedMemEntry::SharedMemEntry(vpn_t start, vpn_t end, int prot, int flags, int fd, ssize_t offset, bool shared, FileDescriptor* globalFileDescriptor)
    : start_(start),end_(end), prot_(prot), flags_(flags), fd_(fd), offset_(offset), shared_(shared), globalFileDescriptor_(globalFileDescriptor)
{
}

SharedMemEntry::SharedMemEntry(const SharedMemEntry& other)
{
    start_ = other.start_;
    end_ = other.end_;
    prot_ = other.prot_;
    flags_ = other.flags_;
    fd_ = other.fd_;
    offset_ = other.offset_;
    shared_ = other.shared_;
    globalFileDescriptor_ = other.globalFileDescriptor_;
}

bool SharedMemEntry::isInBlockRange(vpn_t vpn)
{
    return vpn >= start_ && vpn <= end_;
}

size_t SharedMemEntry::getSize()
{
    return end_ - start_ + 1;
}

ssize_t SharedMemEntry::getOffset(size_t vpn)
{
    assert(isInBlockRange(vpn) && "SharedMemEntry::getOffset: vpn not in range\n");
    return offset_ + (vpn - start_) * PAGE_SIZE;
}


/////////////////////// SharedMemManager ///////////////////////


SharedMemManager::SharedMemManager()
    : shared_mem_lock_("shared_mem_lock_")
{
    last_free_vpn_ = (vpn_t) MIN_SHARED_MEM_VPN;
}

SharedMemManager::SharedMemManager(const SharedMemManager& other)
    : last_free_vpn_(other.last_free_vpn_), shared_mem_lock_("shared_mem_lock_")
{
    for (auto it : other.shared_map_)
    {
        shared_map_.push_back(new SharedMemEntry(*it));
    }
}

SharedMemManager::~SharedMemManager()
{
    for (auto it : shared_map_)
    {
        delete it;
    }
    shared_map_.clear();
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

    if ( (flags == (MAP_ANONYMOUS | MAP_PRIVATE)) || (flags == MAP_PRIVATE && fd >= 0) )
    {
        retval = addEntry(start, length, prot, flags, fd, offset, false);
    }
    else if ((flags == (MAP_ANONYMOUS | MAP_SHARED)) || (flags == MAP_SHARED && fd >= 0) )
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
    // error checking: entry already exist, lock shared_mem_, out of free shared mem, given length is too big
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
    if (end > MAX_SHARED_MEM_VPN)
    {
        debug(ERROR_DEBUG, "SharedMemManager::addEntry: failed, size required is bigger than the available free pages \n");
        return MAP_FAILED;
    }


    FileDescriptor* globalFileDescriptor = nullptr;

    if (fd >= 0) {
      UserThread& currentUserThread = *((UserThread*)currentThread);
      UserProcess& current_process = *currentUserThread.process_;
      LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;
      lfdTable.lfds_lock_.acquire();

      LocalFileDescriptor* localFileDescriptor = lfdTable.getLocalFileDescriptor(fd);
      assert( localFileDescriptor != nullptr && "SharedMemManager::addEntry: localFileDescriptor is null\n");
      globalFileDescriptor = localFileDescriptor->getGlobalFileDescriptor();
      globalFileDescriptor ->incrementRefCount();  // check if locking is required around incrementRefCount()
      debug(SYSCALL, "addEntry: Reference count after increment: %d\n", globalFileDescriptor ->getRefCount());

      lfdTable.lfds_lock_.release();
    }

    debug(MMAP, "SharedMemManager::mmap: global fd: %u\n", globalFileDescriptor->getFd());


    assert(globalFileDescriptor->getFd() != 0 && "Global file descriptor pointer is null");assert(globalFileDescriptor != nullptr && "SharedMemManager::addEntry: globalFileDescriptor is null\n");


    // actually adding entry
    SharedMemEntry* entry = new SharedMemEntry(start, end, prot, flags, fd, offset, shared, globalFileDescriptor);
    shared_map_.push_back(entry);

    void* start_addr = (void*) (start * PAGE_SIZE);
    last_free_vpn_ += size;

    // if the page is shared, add it to usp vector of IPTManager
    if (shared)
    {
        IPTManager* ipt = IPTManager::instance();
        ArchMemory* arch_memory = &((UserThread*) currentThread)->loader_->arch_memory_;
        ipt->fake_ppn_lock_.acquire();
        for (vpn_t vpn = start; vpn <= end; vpn++)
        {
            ipt->insertFakePpnEntry(arch_memory, vpn);
        }
        ipt->fake_ppn_lock_.release();
    }


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
    assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread && "SharedMemManager::handleSharedPF: IPT need to be locked");
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::handleSharedPF: shared_mem_lock_ not held\n");
    assert(currentThread->loader_->arch_memory_.archmemory_lock_.isHeldBy(currentThread) && "SharedMemManager::handleSharedPF: archmemory_lock_ not held\n");

    ArchMemory* arch_memory = &currentThread->loader_->arch_memory_;
    SharedMemEntry* entry = getSharedMemEntry(address);

    // create new ppn and copy content from fd if necessary
    size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);
    size_t vpn = address / PAGE_SIZE;
    if ((entry->flags_ == MAP_PRIVATE || entry->flags_ == MAP_SHARED) && entry->fd_ >= 0)
    {
        debug(MMAP, "SharedMemManager::handleSharedPF: fd exists, copying content from fd (%d) to the new ppn (%zu)\n", entry->fd_, ppn);
        ssize_t offset = entry->getOffset(vpn);
        copyContentFromFD(ppn, entry->fd_, offset, arch_memory, entry->globalFileDescriptor_);
        
    }
    
    // map every relevant archmem to the new ppn
    if (entry->shared_)
    {
        IPTManager* ipt = IPTManager::instance();
        ipt->fake_ppn_lock_.acquire();

        debug(MMAP, "SharedMemManager::handleSharedPF: Mapping the new shared ppn %zu to every relevant archmem\n", ppn);
        ipt->mapRealPPN(ppn, vpn, arch_memory, preallocated_pages);

        ipt->fake_ppn_lock_.release();

    }
    else
    {
        bool rv = arch_memory->mapPage(vpn, ppn, 1, preallocated_pages);
        assert(rv == true);
        setProtectionBits(entry, arch_memory, vpn);
    }
    
    

    debug(MMAP, "SharedMemManager::handleSharedPF: done\n");
}

void SharedMemManager::copyContentFromFD(size_t ppn, int fd, ssize_t offset, ArchMemory* arch_memory, FileDescriptor *globalFileDescriptor)
{
    debug(MMAP, "SharedMemManager::copyContentFromFD: with ppn %zu, fd %d, offset %ld\n", ppn, fd, offset);
    assert(fd != fd_stdin && "invalid fd for this operation");
    assert(arch_memory && "SharedMemManager::copyContentFromFD: arch_memory is null\n");
    
    UserThread& currentUserThread = *((UserThread*)currentThread);
    UserProcess& current_process = *currentUserThread.process_;
    LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

    // get the pointer of the page we want to copy to
    pointer target = arch_memory->getIdentAddressOfPPN(ppn);


    lfdTable.lfds_lock_.acquire();

    // read file and write to page
    FileDescriptor* global_fd_obj = globalFileDescriptor;
    debug(MMAP, "SharedMemManager::mmap1111111111111111: global fd: %u\n", global_fd_obj->getFd());

    assert(global_fd_obj != nullptr && "Global file descriptor pointer is null");
    assert(global_fd_obj != 0 && "Global file descriptor pointer is null");
    assert(global_fd_obj->getType() != FileDescriptor::FileType::PIPE && "SharedMemManager::copyContentFromFD: cannot copy content from a pipe\n");

    size_t global_fd = global_fd_obj->getFd();
    if (VfsSyscall::lseek(global_fd, offset, SEEK_SET) == (l_off_t) -1)
    {
        assert(false && "SharedMemManager::copyContentFromFD: lseek failed\n");
    }

    int32 num_read = VfsSyscall::read(global_fd, (char *) target, PAGE_SIZE);
    if (num_read == -1)
    {
        assert(false && "SharedMemManager::copyContentFromFD: read failed\n");
    }
    debug(FILEDESCRIPTOR, "SharedMemManager::copyContentFromFD: read %d bytes from fd %d (global fd %zu)\n", num_read, fd, global_fd);

    lfdTable.lfds_lock_.release();

    debug(MMAP, "SharedMemManager::copyContentFromFD: copied %d bytes from fd (%d) to page (%zu)\n", num_read, fd, ppn);
}

void SharedMemManager::setProtectionBits(SharedMemEntry* entry, ArchMemory* archmem, size_t vpn)
{
    int read = 0;
    int write = 0;
    int execute = 0;
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

    archmem->setProtectionBits(vpn, read, write, execute);

}


SharedMemEntry* SharedMemManager::getSharedMemEntry(size_t address)
{
    // assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::getSharedMemEntry: shared_mem_lock_ not held\n");

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

    
    // check if the munmap is valid, or already unmapped by some faster thread
    ustl::vector<ustl::pair<vpn_t, SharedMemEntry*>> relevant_pages;
    findRelevantPages(relevant_pages, (size_t)start, length);
    if (relevant_pages.empty())
    {
        debug(ERROR_DEBUG, "SharedMemManager::munmap: the given range includes pages that are not shared mem, or pages were already unmapped by someone else\n");
        archmem_lock->release();
        ipt_lock->release();
        shared_mem_lock_.release();
        return -1;
    }

    UserThread& currentUserThread = *((UserThread*)currentThread);
    UserProcess& current_process = *currentUserThread.process_;
    LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

    lfdTable.lfds_lock_.acquire();
    assert( relevant_pages[0].second->globalFileDescriptor_ != nullptr && "SharedMemManager::munmap: relevant_pages[0].second->globalFileDescriptor_ is nullptr\n");
    // If there is a valid global file descriptor related to this shared memory entry
    FileDescriptor* globalFileDescriptor = relevant_pages[0].second->globalFileDescriptor_;
    debug(MMAP, "SharedMemManager::munmap2222222222222222222222222222222: global fd: %u\n", globalFileDescriptor->getFd());

    assert(globalFileDescriptor != 0 && "Global file descriptor pointer is null");


    lfdTable.lfds_lock_.release();

    // unmapping each page
    for (auto it : relevant_pages)
    {
      unmapOnePage(it.first, it.second, globalFileDescriptor);
    }



    lfdTable.lfds_lock_.acquire();
    globalFileDescriptor->decrementRefCount(); // ensure that locking is required around decrementRefCount()

    debug(SYSCALL, "munmap: Reference count after decrement: %d\n", globalFileDescriptor ->getRefCount());

    // If reference count drops to 0, delete the global file descriptor
    if (globalFileDescriptor->getRefCount() == 0) {
      // Pass on your method to delete Global File Descriptor
      LocalFileDescriptorTable::deleteGlobalFileDescriptor(globalFileDescriptor);
    }

    lfdTable.lfds_lock_.release();


    archmem_lock->release();
    ipt_lock->release();
    shared_mem_lock_.release();

    return 0;
}

void SharedMemManager::findRelevantPages(ustl::vector<ustl::pair<vpn_t, SharedMemEntry*>> &relevant_pages, size_t start, size_t length)
{
    debug(MMAP, "SharedMemManager::findRelevantPages: start: %zu, length: %zu. Looping through all shared mem and find relevant pages to unmap\n", start, length);
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::findRelevantPages: shared_mem_lock_ not held\n");
    assert(start >= MIN_SHARED_MEM_ADDRESS && start < MAX_SHARED_MEM_ADDRESS && "SharedMemManager::findRelevantPages: start out of range\n");
    assert(length > 0 && "SharedMemManager::findRelevantPages: invalid length\n");
    assert(relevant_pages.empty() && "SharedMemManager::findRelevantPages: relevant_pages not empty\n");

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
                assert(start_vpn <= end_vpn && "SharedMemManager::findRelevantPages: start_vpn > end_vpn\n");
                assert(start_vpn >= MIN_SHARED_MEM_VPN && start_vpn <= MAX_SHARED_MEM_VPN && "SharedMemManager::findRelevantPages: start_vpn becomes out of range\n");
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
                assert(start_vpn <= end_vpn && "SharedMemManager::findRelevantPages: start_vpn > end_vpn\n");
                assert(end_vpn >= MIN_SHARED_MEM_VPN && end_vpn <= MAX_SHARED_MEM_VPN && "SharedMemManager::findRelevantPages: end_vpn becomes out of range\n");
            }
        
        }
    }
    debug(ERROR_DEBUG, "SharedMemManager::findRelevantPages: this shouldn't be reached, the given range is invalid\n");
    relevant_pages.clear();
}

void SharedMemManager::unmapOnePage(vpn_t vpn, SharedMemEntry *sm_entry,
                                    FileDescriptor *pDescriptor) {
    debug(MMAP, "SharedMemManager::unmapOnePage: vpn: %zu\n", vpn);

    // error checking: sharedmem, ipt and archmem is lock. the given vpn and sm_entry exists and related to each other. 
    ArchMemory* arch_memory = &((UserThread*) currentThread)->loader_->arch_memory_;
    assert(sm_entry && "SharedMemManager::unmapOnePage: invalid sm_entry\n");
    assert(sm_entry->getSize() > 0 && "SharedMemManager::unmapOnePage: sm_entry size is 0\n");
    auto it = ustl::find(shared_map_.begin(), shared_map_.end(), sm_entry);
    assert(it != shared_map_.end() && "SharedMemManager::unmapOnePage: sm_entry not found in shared_map_\n");
    if (sm_entry->start_ > vpn || sm_entry->end_ < vpn)
    {
        assert(false && "SharedMemManager::unmapOnePage: vpn not in range of sm_entry\n");
    }
    assert(shared_mem_lock_.isHeldBy((Thread*) currentThread) && "SharedMemManager::unmapOnePage: shared_mem_lock_ not held\n");
    assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread && "SharedMemManager::unmapOnePage: IPT need to be locked");
    assert(arch_memory->archmemory_lock_.heldBy() == currentThread && "SharedMemManager::unmapOnePage: archmemory_lock_ not held\n");
    
    // unmap and write to file if necessary
    if (arch_memory->checkAddressValid(vpn * PAGE_SIZE))
    {
        debug(MMAP, "SharedMemManager::unmapOnePage: unmapping page vpn %zu\n", vpn);
        
        if (isTimeToWriteBack(sm_entry, arch_memory, vpn))
        {
            debug(MMAP, "SharedMemManager::unmapOnePage: private page with fd, writing back to file\n");
            ssize_t offset = sm_entry->getOffset(vpn);
            debug(MMAP, "SharedMemManager::mmap44444444444444444444444444444444444: global fd: %u\n", pDescriptor->getFd());
            assert(pDescriptor != 0 && "Global file descriptor pointer is null");
            writeBackToFile(vpn, sm_entry->fd_, offset, arch_memory, pDescriptor);
        }
        arch_memory->unmapPage(vpn);
    }
    else if (sm_entry->shared_) // if page is not valid, but shared, we need to remove the entry from IPT::fake_ppn_map_
    {
        IPTManager* ipt = IPTManager::instance();
        ipt->fake_ppn_lock_.acquire();
        ipt->unmapOneFakePPN(vpn, arch_memory);
        ipt->fake_ppn_lock_.release();
    }

    // remove entry from shared_map (split the memory segment if necessary)
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
                                                    sm_entry->fd_, sm_entry->offset_, sm_entry->shared_, sm_entry->globalFileDescriptor_));
            shared_map_.push_back(new SharedMemEntry(vpn + 1, sm_entry->end_, sm_entry->prot_, sm_entry->flags_, 
                                                    sm_entry->fd_, sm_entry->offset_, sm_entry->shared_, sm_entry->globalFileDescriptor_));
            shared_map_.erase(it);
            delete sm_entry;
        }
    }
    
}

bool SharedMemManager::isTimeToWriteBack(SharedMemEntry* sm_entry, ArchMemory* arch_memory, size_t vpn)
{
    bool shared_flag = (sm_entry->flags_ == MAP_SHARED && sm_entry->fd_ >= 0);

    ArchMemoryMapping m = arch_memory->resolveMapping(vpn);
    PageTableEntry* pte = &m.pt[m.pti];
    size_t ppn = pte->page_ppn;
    bool last_process = PageManager::instance()->getReferenceCount(ppn) == 1;

    return shared_flag && last_process;
}

void SharedMemManager::unmapAllPages(ArchMemory* arch_memory)
{
    debug(MMAP, "SharedMemManager::unmapAllPages unmaping all shared mem pages\n");

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
                if (isTimeToWriteBack(it, arch_memory, vpn))
                {
                    debug(MMAP, "SharedMemManager::unmapOnePage: private page with fd, writing back to file\n");
                    ssize_t offset = it->getOffset(vpn);
                    FileDescriptor* global_fd_obj = it->globalFileDescriptor_; // retrieve the global file descriptor
                    debug(MMAP, "SharedMemManager::mmap33333333333333333333333333: global fd: %u\n", global_fd_obj->getFd());
                    assert(global_fd_obj->getFd() != 0 && "Global file descriptor pointer is null");
                    writeBackToFile(vpn, it->fd_, offset, arch_memory, global_fd_obj);
                }
                arch_memory->unmapPage(vpn);
            }
        }
    }
    arch_memory->archmemory_lock_.release();
    IPTManager::instance()->IPT_lock_.release();
    shared_mem_lock_.release();
    debug(MMAP, "SharedMemManager::unmapAllPages: done\n");
}


void SharedMemManager::writeBackToFile(size_t vpn, int fd, ssize_t offset, ArchMemory* arch_memory, FileDescriptor *globalFileDescriptor)
{
    debug(MMAP, "SharedMemManager::writeBackToFile: vpn %zu, fd %d, offset %ld\n", vpn, fd, offset);
    assert(arch_memory && "SharedMemManager::writeBackToFile: arch_memory is null\n");
    assert(fd != fd_stdout && "invalid fd for this operation");

    UserThread& currentUserThread = *((UserThread*)currentThread);
    UserProcess& current_process = *currentUserThread.process_;
    LocalFileDescriptorTable& lfdTable = current_process.localFileDescriptorTable;

    // get pointer to the source page we want to copy from
    ArchMemoryMapping m = arch_memory->resolveMapping(vpn);
    PageTableEntry* pte = &m.pt[m.pti];
    size_t ppn = pte->page_ppn;
    pointer source = arch_memory->getIdentAddressOfPPN(ppn);

    // write to file
    lfdTable.lfds_lock_.acquire();

    FileDescriptor *global_fd_obj = globalFileDescriptor;
    assert(global_fd_obj != nullptr && "Global file descriptor pointer is null");
    assert(global_fd_obj->getType() != FileDescriptor::FileType::PIPE && "SharedMemManager::writeBackToFile: cannot write back to a pipe with this operation\n");

    size_t global_fd = global_fd_obj->getFd();
    assert(global_fd_obj->getFd() != 0 && "Global file descriptor pointer is null");
    if (VfsSyscall::lseek(global_fd, offset, SEEK_SET) == (l_off_t) -1)
    {
      debug(MMAP, "SharedMemManager::writeBackToFile: lseek failed on global fd: %zu offset: %ld\n", global_fd, offset);
      assert(false && "SharedMemManager::writeBackToFile: lseek failed\n");
    }

    int32 num_written = VfsSyscall::write(global_fd, (char*) source, PAGE_SIZE);
    if (num_written == -1)
    {
      assert(false && "SharedMemManager::writeBackToFile: write failed\n");
    }

    lfdTable.lfds_lock_.release();

    debug(MMAP, "SharedMemManager::writeBackToFile: wrote %d bytes from vpn %zu (ppn %zu) to fd %d (global fd %zu)\n", num_written, vpn, ppn, fd, global_fd);
}

