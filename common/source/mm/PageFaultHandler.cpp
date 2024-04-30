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
  }
  else if(!user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing an invalid kernel address.\n");
  }
  else if(user && address >= USER_BREAK)
  {
    debug(PAGEFAULT, "You are accessing a kernel address in user-mode.\n");
  }
  else if(present)
  {
    if (currentThread->loader_->isCOW(address))
    {
      debug(PAGEFAULT_TEST, "pagefault even though the address is mapped BUT ITS COW.\n");
      return 3;
    }
    else
    {
      debug(PAGEFAULT, "You got a pagefault even though the address is mapped.\n");
    }
  }
  else if(user && !present && 
          address > null_reference_check_border_ && address < USER_BREAK)
  {
    debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Checking if its a growing stack %p \n", (void*)address);
    UserSpaceMemoryManager* manager = ((UserThread*) currentThread)->process_->user_mem_manager_;
    assert(manager && "UserSpaceMemoryManager is not initialized.");
    int retval = manager->checkValidGrowingStack(address);
    
    if(retval == 11)  // corruption detected
    {
      debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Segmentation fault detected. Exiting with error 11\n");
      return 0;
    }
    else if(retval == 1)  // valid growing stack
    {
      return 69;
    }
    debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: This page fault is not related to growing stack \n");
    return 1;
  }
  else
  {
    // everything seems to be okay
    return 1;
  }
  return 0;
}

inline void PageFaultHandler::handlePageFault(size_t address, bool user,
                                          bool present, bool writing,
                                          bool fetch, bool switch_to_us)
{
  if (PAGEFAULT & OUTPUT_ENABLED)
    kprintfd("\n");
  debug(PAGEFAULT, "Address: %18zx - Thread %zu: %s (%p)\n",
        address, currentThread->getTID(), currentThread->getName(), currentThread);
  debug(PAGEFAULT, "Flags: %spresent, %s-mode, %s, %s-fetch, switch to userspace: %1d\n",
        present ? "    " : "not ",
        user ? "  user" : "kernel",
        writing ? "writing" : "reading",
        fetch ? "instruction" : "    operand",
        switch_to_us);

  ArchThreads::printThreadRegisters(currentThread, false);

  int status = checkPageFaultIsValid(address, user, present, switch_to_us);
  if (status == 1) // everything seems to be okay, no page fault
  {
    currentThread->loader_->loadPage(address);
  }
  else if (status == 3)
  {
    if(writing && currentThread->loader_->isCOW(address)) //bit of entry->writable = =1?
    {

      debug(PAGEFAULT_TEST, "is COW, copying Page\n");
      currentThread->loader_->copyPage(address);
    }
  }
  else if (status == 69)
  {
    debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Growing stack is valid. Creating new stack for current thread\n");
    UserSpaceMemoryManager* manager = ((UserThread*) currentThread)->process_->user_mem_manager_;
    assert(manager && "UserSpaceMemoryManager is not initialized.");
    int status = manager->increaseStackSize(address);
    if (status == -1)
    {
      debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Could not increase stack size.\n");
      if (currentThread->loader_)
        Syscall::exit(9999);
      else
        currentThread->kill();
      }
    else
    {
      debug(GROW_STACK, "PageFaultHandler::checkPageFaultIsValid: Stack size increased successfully\n");
    }
  }
  else
  {
    // the page-fault seems to be faulty, print out the thread stack traces
    ArchThreads::printThreadRegisters(currentThread, true);
    currentThread->printBacktrace(true);
    if (currentThread->loader_)
      Syscall::exit(9999);
    else
      currentThread->kill();
  }
  debug(PAGEFAULT, "Page fault handling finished for Address: %18zx.\n", address);
}

void PageFaultHandler::enterPageFault(size_t address, bool user,
                                      bool present, bool writing,
                                      bool fetch)
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
