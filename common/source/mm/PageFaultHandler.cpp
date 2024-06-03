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
    return PRESENT;
  }
  // else if(user)            //TODOs: growingstack disabled for now - when adding again make sure that it works in combination with swapping
  // {
  //   return USER;
  // }
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

  ArchMemory& current_archmemory = currentThread->loader_->arch_memory_;
   UserSpaceMemoryManager* heap_manager = ((UserThread*) currentThread)->process_->user_mem_manager_;
  assert(current_archmemory.archmemory_lock_.heldBy() != currentThread && "Archmemory lock should not be held on pagefault");

  if (currentThread->holding_lock_list_)
  {
    debug(PAGEFAULT, "PageFaultHandler::handlePageFault: currentThread still holding lock %s\n", currentThread->holding_lock_list_->getName());
    assert(!currentThread->holding_lock_list_ && "PageFaultHandler shouldnt be called while thread still holding lock\n");
  }

  ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAlocatePages(5);  //TODOs make sure that it gets freed in all cases

  int status = checkPageFaultIsValid(address, user, present, switch_to_us);
  if (status == VALID)
  {
    IPTManager::instance()->IPT_lock_.acquire();
    current_archmemory.archmemory_lock_.acquire();

    //Page got set to present in the meantime, so no need to do anything anymore
    if(current_archmemory.isPresent(address))
    {
      debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Swapped out detected, but another thread already swap this page in. Do nothing\n"); 
      current_archmemory.archmemory_lock_.release();
      IPTManager::instance()->IPT_lock_.release();
    }
    //Page is swapped out
    else if(current_archmemory.isSwapped(address))
    {
      debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Swapped out detected. Requesting a swap in\n");
      size_t vpn = address / PAGE_SIZE;
      size_t disk_offset = current_archmemory.getDiskLocation(vpn);
      if(DIRECT_SWAPPING)
      {
        current_archmemory.archmemory_lock_.release();
        SwappingManager::instance()->swapInPage(disk_offset, preallocated_pages);
        IPTManager::instance()->IPT_lock_.release();
      }
      else if(ASYNCHRONOUS_SWAPPING)
      {
        SwappingThread* swapper = &Scheduler::instance()->swapping_thread_;
        current_archmemory.archmemory_lock_.release();  //TODOs: we need to check if ipt entry still exist and page is still swapped out !!!
        IPTManager::instance()->IPT_lock_.release();

        swapper->swap_in_lock_.acquire();
        swapper->addSwapIn(disk_offset, &preallocated_pages);
        while (swapper->isOffsetInMap(disk_offset))               //TODOs not check if offset is in map but check if at offset in disk map is this vpn and archmemory - means only one thread can swap in
        {
          swapper->swap_in_cond_.wait();
        }
        swapper->swap_in_lock_.release();
        debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Page swapped in successful (from offset %zu)\n", disk_offset);
      }
      else
      {
        debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Try to swap in page even though swapping is disabled.\n");
        current_archmemory.archmemory_lock_.release();
        IPTManager::instance()->IPT_lock_.release();
        assert(0);
      }
    }
    //Page is on heap
    else if(address >= heap_manager->heap_start_ && address < heap_manager->current_break_)  //TODOs needs to be locked
    {
      size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);
      size_t vpn = address / PAGE_SIZE;
      bool rv = currentThread->loader_->arch_memory_.mapPage(vpn, ppn, 1, preallocated_pages);
      assert(rv == true);
      current_archmemory.archmemory_lock_.release();
      IPTManager::instance()->IPT_lock_.release();
    }
    //Page needs to be loader from binary 
    else
    {
      debug(PAGEFAULT, "PageFaultHandler::checkPageFaultIsValid: Page is not present and not swapped out -> Loading page\n");
      currentThread->loader_->loadPage(address, preallocated_pages);
      current_archmemory.archmemory_lock_.release();
      IPTManager::instance()->IPT_lock_.release();
    }
  }
  else if (status == PRESENT)
  {
    IPTManager::instance()->IPT_lock_.acquire();
    current_archmemory.archmemory_lock_.acquire();
    //Page is not present anymore we need to swap it in -> do nothing so it gets swapped in on the next pagefault
    if(!current_archmemory.isPresent(address))
    {
      //SwappingManager::instance()->swapInPage(..., preallocated_pages);       //TODOs: check if it better with or without comment out
    }
    //Page is set readonly we want to write and cow-bit is set -> copy page
    else if(writing && current_archmemory.isCOW(address) && !current_archmemory.isWriteable(address))
    {
      current_archmemory.copyPage(address, preallocated_pages);
    }
    //Page is set writable we want to write and cow-bit is set -> sombody else was faster with cow
    else if(writing && current_archmemory.isCOW(address) && current_archmemory.isWriteable(address))
    {

    }
    //We want to write to a page that is readable and not cow -> error
    else
    {
      current_archmemory.archmemory_lock_.release();
      IPTManager::instance()->IPT_lock_.release();
      PageManager::instance()-> releaseNotNeededPages(preallocated_pages);
      errorInPageFaultKillProcess();
    }
    
    current_archmemory.archmemory_lock_.release();
    IPTManager::instance()->IPT_lock_.release();
  }
  else  //status INVALID
  {
    PageManager::instance()->releaseNotNeededPages(preallocated_pages);
    errorInPageFaultKillProcess();
  }

  PageManager::instance()->releaseNotNeededPages(preallocated_pages);
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



int PageFaultHandler::checkGrowingStack(size_t address)
{
    debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Checking if its a growing stack %p \n", (void*)address);
    UserSpaceMemoryManager* manager = ((UserThread*) currentThread)->process_->user_mem_manager_;
    assert(manager && "UserSpaceMemoryManager is not initialized.");

    int status = manager->checkValidGrowingStack(address);
        
    if(status != GROWING_STACK_VALID)
    {
      return status;
    }

    status = manager->increaseStackSize(address);
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

//Code for growing stack:

  // else if (status == USER)                //TODOs: Does not work in combination with swapping - add in again later
  // {
    // IPTManager::instance()->IPT_lock_.acquire();
    // currentThread->loader_->arch_memory_.archmemory_lock_.acquire();
    // int retval = checkGrowingStack(address);
    // currentThread->loader_->arch_memory_.archmemory_lock_.release();
    //  IPTManager::instance()->IPT_lock_.release();
    // if (retval == GROWING_STACK_FAILED)
    // {
    //   debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Could not increase stack size.\n");
    //   errorInPageFaultKillProcess();
    // }
    // else if(retval == NOT_RELATED_TO_GROWING_STACK)
    // {
    //   debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: This page fault is not related to growing stack \n");
    //   currentThread->loader_->loadPage(address);
    // }
    // else
    // {
    //   assert(retval == GROWING_STACK_VALID);
    //   debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Stack size increased successfully\n");
    // }
  // }

