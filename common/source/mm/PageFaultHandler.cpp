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

inline int PageFaultHandler::checkPageFaultIsValid(size_t address, bool user,
                                                    bool present, bool switch_to_us)
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

  if (currentThread->holding_lock_list_)
  {
    debug(PAGEFAULT, "PageFaultHandler::handlePageFault: currentThread still holding lock %s\n", currentThread->holding_lock_list_->getName());
    assert(!currentThread->holding_lock_list_ && "PageFaultHandler shouldnt be called while thread still holding lock\n");
  }
  // assert(current_archmemory.archmemory_lock_.heldBy() != currentThread && "Archmemory lock should not be held on pagefault");

  ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAlocatePages(5);

  int status = checkPageFaultIsValid(address, user, present, switch_to_us);
  if (status == VALID)
  {
    ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAlocatePages(4);  // loadPage() needs 4 and swapInPage needs 1. 

    IPTManager::instance()->IPT_lock_.acquire();
    current_archmemory.archmemory_lock_.acquire();

    //Page got set to present in the meantime
    if(current_archmemory.isPresent(address))
    {
    }
    //Page is swapped out
    else if(current_archmemory.isSwapped(address))
    {
      SwappingManager::instance()->swapInPage(address / PAGE_SIZE, preallocated_pages);
    }
    //Page needs to be loader from binary 
    else
    {
      currentThread->loader_->loadPage(address, preallocated_pages);
    }
    current_archmemory.archmemory_lock_.release();
    IPTManager::instance()->IPT_lock_.release();

    PageManager::instance()-> releaseNotNeededPages(preallocated_pages);
  }
  else if (status == PRESENT)
  {
    ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAlocatePages(1);  // copyPage() and swapInPage each only needs 1 alloc
    IPTManager::instance()->IPT_lock_.acquire();
    current_archmemory.archmemory_lock_.acquire();
    //Page is not present anymore we need to swap it in
    if(!current_archmemory.isPresent(address))
    {
      SwappingManager::instance()->swapInPage(address / PAGE_SIZE, preallocated_pages);  //TODOs: maybe cow
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
    
    PageManager::instance()-> releaseNotNeededPages(preallocated_pages);
  }
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
  else  //status INVALID
  {
    PageManager::instance()-> releaseNotNeededPages(preallocated_pages);
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

