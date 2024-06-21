#include "PageFaultHandler.h"
#include "kprintf.h"
#include "Thread.h"
#include "ArchInterrupts.h"
#include "offsets.h"
#include "Scheduler.h"
#include "Loader.h"
#include "Syscall.h"
#include "ArchThreads.h"
#include "UserSpaceMemoryManager.h"
#include "UserThread.h"
#include "UserProcess.h"
#include "SwappingManager.h"

#include "PageManager.h"

extern "C" void arch_contextSwitch();

const size_t PageFaultHandler::null_reference_check_border_ = PAGE_SIZE;

inline int PageFaultHandler::checkPageFaultIsValid(size_t address, bool user, bool present, bool switch_to_us)
{
  assert((user == switch_to_us) && "Thread is in user mode even though is should not be.");
  assert(!(address < USER_BREAK && currentThread->loader_ == 0) && "Thread accesses the user space, but has no loader.");
  assert(!(user && currentThread->user_registers_ == 0) && "Thread is in user mode, but has no valid registers.");

  if(address < null_reference_check_border_)
  {
    debug(PAGEFAULT, "Maybe you are dereferencing a null-pointer.\n");
    return INVALID;
  }
  else if(!user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing an invalid kernel address.\n");
    return INVALID;
  }
  else if(user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing a kernel address in user-mode.\n");
    return INVALID;
  }
  else if(present)
  {
    debug(PAGEFAULT, "You got a pagefault even though the address is mapped.\n");
    return IS_PRESENT;
  }
  else if(user)
  {
    return USER;
  }
  else
  {
    // everything seems to be okay
    return VALID;
  }
  return INVALID;
}

inline void PageFaultHandler::handlePageFault(size_t address, bool user, bool present, bool writing, bool fetch, bool switch_to_us)
{
  if (PAGEFAULT & OUTPUT_ENABLED)
    kprintfd("\n");
  debug(PAGEFAULT, "Address: %18zx - Thread %zu: %s (%p)\n", address, currentThread->getTID(), currentThread->getName(), currentThread);
  debug(PAGEFAULT, "Flags: %spresent, %s-mode, %s, %s-fetch, switch to userspace: %1d\n", present ? "    " : "not ",
        user ? "  user" : "kernel", writing ? "writing" : "reading",  fetch ? "instruction" : "    operand", switch_to_us);

  ArchThreads::printThreadRegisters(currentThread, false);


  if (currentThread->holding_lock_list_)
  {
    debug(PAGEFAULT, "PageFaultHandler::handlePageFault: currentThread still holding lock %s\n", currentThread->holding_lock_list_->getName());
    assert(!currentThread->holding_lock_list_ && "PageFaultHandler shouldnt be called while thread still holding lock\n");
  }

  int status = checkPageFaultIsValid(address, user, present, switch_to_us);

  if (status == VALID)
  {
    handleValidPageFault(address);
  }
  else if (status == IS_PRESENT)
  {
    handlePresentPageFault(address, writing);
  }
  else if(status == USER)
  {
    handleUserPageFault(address);
  }
  else  //status INVALID
  {
    errorInPageFaultKillProcess();
  }

  debug(PAGEFAULT, "Page fault handling finished for Address: %18zx.\n", address);
}

void PageFaultHandler::enterPageFault(size_t address, bool user, bool present, bool writing, bool fetch)
{
  assert(currentThread && "You have a pagefault, but no current thread");
  //save previous state on stack of currentThread
  uint32 saved_switch_to_userspace = currentThread->switch_to_userspace_;

  currentThread->switch_to_userspace_ = 0;
  currentThreadRegisters = currentThread->kernel_registers_;
  ArchInterrupts::enableInterrupts();

  handlePageFault(address, user, present, writing, fetch, saved_switch_to_userspace);

  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  if (currentThread->switch_to_userspace_)
    currentThreadRegisters = currentThread->user_registers_;
}



int PageFaultHandler::checkGrowingStack(size_t address, ustl::vector<uint32>& preallocated_pages)
{
    debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Checking if its a growing stack %p \n", (void*)address);
    UserSpaceMemoryManager* manager = ((UserThread*) currentThread)->process_->user_mem_manager_;
    assert(manager && "UserSpaceMemoryManager is not initialized.");

    int status = manager->checkValidGrowingStack(address);
        
    if(status != GROWING_STACK_VALID)
    {
      return status;
    }

    status = manager->increaseStackSize(address, preallocated_pages);
    return status;
}

void PageFaultHandler::errorInPageFaultKillProcess()
{
    // the page-fault seems to be faulty, print out the thread stack traces
    ArchThreads::printThreadRegisters(currentThread, true);
    currentThread->printBacktrace(true);
    if (currentThread->loader_)
    {
       Syscall::exit(9999);
    }
    else
    {
      currentThread->kill();
    }   
}


bool allLockRealeased(Mutex* shared_mem_lock, Mutex* heap_lock, Mutex* swap_lock, Mutex* archmem_lock, Mutex* ipt_lock)
{
  return !shared_mem_lock->isHeldBy(currentThread) && !heap_lock->isHeldBy(currentThread) && !swap_lock->isHeldBy(currentThread) &&
         !archmem_lock->isHeldBy(currentThread) && !ipt_lock->isHeldBy(currentThread);
}


void PageFaultHandler::handleValidPageFault(size_t address)
{
  debug(PAGEFAULT, "PageFaultHandler::handleValidPageFault: Handling valid page fault\n");

  ArchMemory& current_archmemory = currentThread->loader_->arch_memory_;
  UserSpaceMemoryManager* heap_manager = ((UserThread*) currentThread)->process_->user_mem_manager_;
  SwappingThread* swapper = &Scheduler::instance()->swapping_thread_;
  SharedMemManager* smm = heap_manager->shared_mem_manager_;
  Mutex* swap_lock = &swapper->swap_in_lock_;
  Mutex* heap_lock = &heap_manager->current_break_lock_;
  Mutex* ipt_lock = &IPTManager::instance()->IPT_lock_;
  Mutex* archmem_lock = &current_archmemory.archmemory_lock_;
  Mutex* shared_mem_lock = &smm->shared_mem_lock_;

  // swap-in needs 1 page
  // heap needs 3 pages (for mapPage)
  // load from binary needs 4 pages (for loadPage)
  // TODOMINH: change 15 pages to the exact pages we need: count the pages we need in case of shared page
  ustl::vector<uint32> preallocated_pages;
  if((address < MIN_SHARED_MEM_ADDRESS) || (address > MAX_SHARED_MEM_ADDRESS))
  {
    preallocated_pages = PageManager::instance()->preAlocatePages(4);
  }
  else
  {
    preallocated_pages = PageManager::instance()->preAlocatePages(15);
  }


  shared_mem_lock->acquire();
  swap_lock->acquire();
  heap_lock->acquire();
  ipt_lock->acquire();
  archmem_lock->acquire();

  size_t vpn = address/PAGE_SIZE;

  if(current_archmemory.isBitSet(vpn, BitType::PRESENT, false))
  {
    debug(PAGEFAULT, "PageFaultHandler::handleValidPageFault: Another thread already solved this pagefault. Do nothing\n");
    archmem_lock->release();
    ipt_lock->release();
    heap_lock->release();
    swap_lock->release();
    shared_mem_lock->release();
  }
  //Page is swapped out
  else if(current_archmemory.isBitSet(vpn, BitType::SWAPPED_OUT, false))
  {
    heap_lock->release();
    shared_mem_lock->release();
    debug(PAGEFAULT, "PageFaultHandler::handleValidPageFault: Swapped out detected. Requesting a swap in\n");
    size_t disk_offset = current_archmemory.getDiskLocation(vpn);
    assert(disk_offset != 0);

    swapper->addSwapIn(disk_offset, &preallocated_pages);
    archmem_lock->release();
    ipt_lock->release();

    while (swapper->isOffsetInMap(disk_offset))
    {
      swapper->swap_in_cond_.wait();
    }
    swapper->swap_in_lock_.release();
    debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Page swapped in successful (from offset %zu)\n", disk_offset);
  }
  //Page is on heap
  else if(address >= heap_manager->heap_start_ && address < heap_manager->current_break_)
  {
    debug(PAGEFAULT, "PageFaultHandler::handleValidPageFault: Handling pf in Heap\n");
    swap_lock->release();
    shared_mem_lock->release();

      size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);
      bool rv = currentThread->loader_->arch_memory_.mapPage(vpn, ppn, 1, preallocated_pages);
      assert(rv == true);

    archmem_lock->release();
    ipt_lock->release();
    heap_lock->release();
  }
    //Page is on stack
  else if(address > STACK_END && address < STACK_START && current_archmemory.isBitSet(vpn, BitType::DISCARDED, true))
  {
    debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Page %p is on stack.\n", (void*)address);
    swapper->swap_in_lock_.release();
    heap_manager->current_break_lock_.release();
    shared_mem_lock->release();
    size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);
    bool rv = currentThread->loader_->arch_memory_.mapPage(vpn, ppn, 1, preallocated_pages);
    assert(rv == true);
    current_archmemory.archmemory_lock_.release();
    IPTManager::instance()->IPT_lock_.release();
  }
  else if(smm->isAddressValid(address))
  {
    debug(MMAP, "PageFaultHandler::handleValidPageFault: Handling pf in shared memory\n");
    heap_lock->release();
    swap_lock->release();

    smm->handleSharedPF(preallocated_pages, address);

    archmem_lock->release();
    ipt_lock->release();
    shared_mem_lock->release();
  }
  else
  {
    debug(PAGEFAULT, "PageFaultHandler::handleValidPageFault: Page needs to be loaded from binary\n");
    heap_lock->release();
    swap_lock->release();
    shared_mem_lock->release();

    debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Page is not present and not swapped out -> Loading page\n");
    currentThread->loader_->loadPage(address, preallocated_pages);

    archmem_lock->release();
    ipt_lock->release();
  }

  assert(allLockRealeased(shared_mem_lock, heap_lock, swap_lock, archmem_lock, ipt_lock) && "PageFaultHandler::handleValidPageFault: Not all locks are released\n");
  PageManager::instance()->releaseNotNeededPages(preallocated_pages);
}


void PageFaultHandler::handlePresentPageFault(size_t address, bool writing)
{
  debug(PAGEFAULT, "PageFaultHandler::handlePresentPageFault: Handling present page fault\n");

  ArchMemory& current_archmemory = currentThread->loader_->arch_memory_;
  Mutex* ipt_lock = &IPTManager::instance()->IPT_lock_;
  Mutex* archmem_lock = &current_archmemory.archmemory_lock_;
  PageManager* pm = PageManager::instance();

  size_t vpn = address/PAGE_SIZE;

  ustl::vector<uint32> preallocated_pages = pm->preAlocatePages(1);

  ipt_lock->acquire();
  archmem_lock->acquire();
  //Page is not present anymore we need to swap it in -> do nothing so it gets swapped in on the next pagefault
  if(!current_archmemory.isBitSet(vpn, BitType::PRESENT, false))
  {
    debug(PAGEFAULT, "PageFaultHandler::handlePresentPageFault: Page is not present anymore (swapped out). Do nothing\n");
  }
  //Page is set readonly we want to write and cow-bit is set -> copy page
  else if(writing && current_archmemory.isBitSet(vpn, BitType::COW, true) && !current_archmemory.isBitSet(vpn, BitType::WRITEABLE, true))
  {
    debug(PAGEFAULT, "PageFaultHandler::handlePresentPageFault: Page is COW and we want to write. Copy page\n");
    current_archmemory.copyPage(address, preallocated_pages);
  }
  //Page is set writable we want to write -> sombody else was faster with cow
  else if(writing && current_archmemory.isBitSet(vpn, BitType::WRITEABLE, true))
  {
    debug(PAGEFAULT, "PageFaultHandler::handlePresentPageFault: Page is COW but writeable bit is set => Somebody else was faster with COW. Do nothing\n");
  }
    //We want to write to a page that is readable and not cow -> error
  else
  {
    // debug(ERROR_DEBUG, "PageFaultHandler::handlePresentPageFault: Page is not COW (read-only) and we want to write => Error\n");
    // archmem_lock->release();
    // ipt_lock->release();
    // pm-> releaseNotNeededPages(preallocated_pages);
    // errorInPageFaultKillProcess();
  }

  archmem_lock->release();
  ipt_lock->release();

  pm->releaseNotNeededPages(preallocated_pages);
}



//Code for growing stack:

  void PageFaultHandler::handleUserPageFault(size_t address)
  {
    ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAlocatePages(4);
    IPTManager::instance()->IPT_lock_.acquire();
    currentThread->loader_->arch_memory_.archmemory_lock_.acquire();
    int retval = checkGrowingStack(address, preallocated_pages);
    IPTManager::instance()->IPT_lock_.release();
    currentThread->loader_->arch_memory_.archmemory_lock_.release();
    PageManager::instance()->releaseNotNeededPages(preallocated_pages);

    if(retval != GROWING_STACK_VALID)
    {
      handleValidPageFault(address);
    }
  }