#include "UserSpaceMemoryManager.h"
#include "debug.h"
#include "PageManager.h"
#include "ArchMemory.h"
#include "Loader.h"
#include "UserProcess.h"
#include "ArchMemory.h"

#define GUARD_MARKER 0xbadcafe00000ULL  

size_t UserSpaceMemoryManager::totalUsedHeap()
{
  return current_break_ - heap_start_;
}

UserSpaceMemoryManager::UserSpaceMemoryManager(Loader* loader)
  : lock_("UserSpaceMemoryManager::lock_")
{
  heap_start_ = (size_t) loader->getBrkStart();
  current_break_ = heap_start_;
  loader_ = loader;
}

pointer UserSpaceMemoryManager::sbrk(ssize_t size, size_t already_locked)
{
  debug(SBRK, "UserSpaceMemoryManager::sbrk called with size (%zd)\n", size);

  if (!already_locked) {
    lock_.acquire();
    loader_->arch_memory_.lock_.acquire();
  }

  assert(current_break_ + size <= MAX_HEAP_SIZE && "UserSpaceMemoryManager::sbrk: trying to allocate more than MAX_HEAP_SIZE");
  assert(current_break_ + size >= heap_start_ && "UserSpaceMemoryManager::sbrk: trying to deallocate below heap start");

  if(size != 0)
  {
    debug(SBRK, "UserSpaceMemoryManager::sbrk: changing break value\n");
    size_t old_break = current_break_;
    current_break_ = current_break_ + size;

    size_t old_top_vpn = old_break / PAGE_SIZE;
    if ((old_break % PAGE_SIZE) == 0)
      old_top_vpn--;
    size_t new_top_vpn = current_break_ / PAGE_SIZE;
    if ((current_break_ % PAGE_SIZE) == 0)
      new_top_vpn--;

    if(size > 0)
    {
      debug(SBRK, "old break is on page %zx <= new break is on page %zx\n", old_top_vpn, new_top_vpn);
      while(old_top_vpn != new_top_vpn)
      {
        debug(SBRK, "%zx != %zx\n", old_top_vpn, new_top_vpn);
        old_top_vpn++;
        
        size_t new_page = PageManager::instance()->allocPPN();
        if(unlikely(new_page == 0))
        {
          debug(SBRK, "UserSpaceMemoryManager::sbrk: FATAL ERROR, no more physical memory\n");
          current_break_ = old_break;
          if (!already_locked)
          {
            loader_->arch_memory_.lock_.release();
            lock_.release();
          }
          return 0;
        }

        debug(SBRK, "kbsrk: map %zx -> %zx\n", old_top_vpn, new_page);
        void* new_page_ptr = (void*) ArchMemory::getIdentAddressOfPPN(new_page);
        memset(new_page_ptr, 0 , PAGE_SIZE);
        bool successly_mapped = loader_->arch_memory_.mapPage(old_top_vpn, new_page, 1);
        if(unlikely(!successly_mapped))
        {
          debug(SBRK, "UserSpaceMemoryManager::sbrk: FATAL ERROR, could not map page\n");
          current_break_ = old_break;
          if (!already_locked)
          {
            loader_->arch_memory_.lock_.release();
            lock_.release();
          }
          return 0;
        }
      }
    }
    else    // size < 0
    {
      debug(SBRK, "old break is on page %zx >= new break is on page %zx\n", old_top_vpn, new_top_vpn);
      while(old_top_vpn != new_top_vpn)
      {
        bool successly_unmapped = loader_->arch_memory_.unmapPage(old_top_vpn);
        if(unlikely(!successly_unmapped))
        {
          debug(SBRK, "UserSpaceMemoryManager::sbrk: FATAL ERROR, could not unmap page\n");
          current_break_ = old_break;
          if (!already_locked)
          {
            loader_->arch_memory_.lock_.release();
            lock_.release();
          }
          return 0;
        }
        old_top_vpn--;
      }
    }
    debug(SBRK, "UserSpaceMemoryManager::sbrk: break is changed successful, new break value is %zx\n", current_break_);
    if (!already_locked)
    {
      loader_->arch_memory_.lock_.release();
      lock_.release();
    }
    assert(current_break_ >= heap_start_ && "UserSpaceMemoryManager::sbrk: current break is below heap start");
    assert(current_break_ <= MAX_HEAP_SIZE && "UserSpaceMemoryManager::sbrk: current break is above heap limit");
    return (pointer) old_break;
  }
  else
  {
    debug(SBRK, "UserSpaceMemoryManager::sbrk: returning current break value without changing it %zx\n", current_break_);
    pointer old_break = (pointer) current_break_;
    if (!already_locked)
    {
      loader_->arch_memory_.lock_.release();
      lock_.release();
    }
    return old_break;
  }
}


int UserSpaceMemoryManager::brk(size_t new_break_addr)
{
  debug(SBRK, "UserSpaceMemoryManager::brk called with new break address (%zx)\n", new_break_addr);
  assert(new_break_addr >= heap_start_ && "UserSpaceMemoryManager::brk: new break is below heap start");
  assert(new_break_addr <= MAX_HEAP_SIZE && "UserSpaceMemoryManager::brk: new break is above heap limit");

  lock_.acquire();
  loader_->arch_memory_.lock_.acquire();
  ssize_t size = new_break_addr - current_break_;
  pointer resevered_space = sbrk(size, 1);
  if (resevered_space == 0)
  {
    debug(SBRK, "UserSpaceMemoryManager::brk: FATAL ERROR, could not set new break at address (%zx)\n", new_break_addr);
    loader_->arch_memory_.lock_.release();
    lock_.release();
    return -1;
  } 
  else
  {
    debug(SBRK, "UserSpaceMemoryManager::brk: new break is set successful at address (%zx)\n", current_break_);
    loader_->arch_memory_.lock_.release();
    lock_.release();
    return 0;
  }
}


void UserSpaceMemoryManager::initGuard(UserThread* current_thread, size_t top_current_page)
{
  if (!current_thread->guarded_)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: guard_ flag is 0, setting up the guards");
    size_t top_last_page = top_current_page + PAGE_SIZE;
    ArchMemory* arch_memory = &current_thread->process_->loader_->arch_memory_;
    assert(arch_memory->checkAddressValid(top_last_page) && "last page of growing stack is not mapped");

    size_t* guard1 = (size_t*) top_last_page;
    size_t* guard2 = (size_t*) (top_last_page - sizeof(size_t)*5);
    assert(guard1 && "guard1 pointer of the current stack is NULL");
    assert(guard2 && "guard2 pointer of the current stack is NULL");

    if (*guard1 == GUARD_MARKER  && *guard2 == GUARD_MARKER)
    {
      debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: This is 1st growing stack, but guards are already set (likelly by pthread)\n");
      current_thread->guarded_ = 1;
    }
    else
    {
      debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: This is 1st growing stack, setting up the guards\n");
      *guard1 = GUARD_MARKER;
      *guard2 = GUARD_MARKER;
      current_thread->guarded_ = 1;
    }
  }
}

/**
 * @brief check if the address is a valid growing stack address
 * @param address the address to check
 * @return 1 if the address is valid, else 0
*/
int UserSpaceMemoryManager::sanityCheck(size_t address)
{
  if (address < PAGE_SIZE || address > USER_BREAK)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::sanityCheck: address is out of range\n");
    return 0;
  }
  UserThread* current_thread = (UserThread*) currentThread;
  if (address > current_thread->top_stack_ || address < (current_thread->top_stack_ - MAX_STACK_AMOUNT*PAGE_SIZE))
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::sanityCheck: address is not in range of growing stack\n");
    return 0;
  }
  ArchMemory* arch_memory = &((UserThread*) currentThread)->process_->loader_->arch_memory_;
  if (arch_memory->checkAddressValid(address))
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::sanityCheck: address is already mapped\n");
    return 0;
  }
  size_t valid_address = address + PAGE_SIZE;   // address of the last page of stack should be valid
  if (!arch_memory->checkAddressValid(valid_address))
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::sanityCheck: last page is not mapped -> not related to growing stack\n");
    return 0;
  }
  
  return 1;
}


int UserSpaceMemoryManager::checkValidGrowingStack(size_t address)
{
  debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack called with address (%zx)\n", address);
  if (!sanityCheck(address))
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: address failed sanity check\n");
    return 0;
  }
  UserThread* current_thread = (UserThread*) currentThread;
  size_t top_current_page = getTopOfThisPage(address);
  assert(top_current_page && "top_current_page pointer of the current stack is NULL");

  // make sure guard is set up
  debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: passed sanity check, guards are setted\n");
  initGuard(current_thread, top_current_page);

  // get to top of stack where the meta data is stored
  debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: checking if guards are corrupted\n");
  size_t top_current_stack = checkGuardValid(top_current_page);
  if (top_current_stack == 11)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: guards are corrupted. Segfault!!\n");
    return 11;
  }

  finalSanityCheck(address, top_current_stack);
  debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: found a valid top of stack.\n");

  return 1;
}


void UserSpaceMemoryManager::finalSanityCheck(size_t address, size_t top_current_stack)
{
  assert(top_current_stack && "top_current_stack pointer of the current stack is NULL");
  assert(top_current_stack == ((UserThread*) currentThread)->top_stack_ && "this is not our stack");

  assert(address < top_current_stack && "address is not within range of growing stack");
  assert(address > top_current_stack - PAGE_SIZE*MAX_STACK_AMOUNT && "address is not within range of growing stack");
}


int UserSpaceMemoryManager::increaseStackSize(size_t address)
{
  debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize called with address (%zx)\n", address);
  
  // Quick check to see if the address is (somewhat) valid
  size_t top_this_page = getTopOfThisPage(address);
  size_t top_this_stack = checkGuardValid(top_this_page);
  assert(top_this_stack != 11 && "UserSpaceMemoryManager::increaseStackSize: guards are corrupted. Segfault!!");
  finalSanityCheck(address, top_this_stack);

  // Set up new page
  debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize: passed sanity check, setting up new page\n");
  ArchMemory arch_memory = ((UserThread*) currentThread)->process_->loader_->arch_memory_;
  uint64 new_vpn = (top_this_page + sizeof(size_t)) / PAGE_SIZE - 1;
  uint32 new_ppn = PageManager::instance()->allocPPN();
  arch_memory.lock_.acquire();
  bool page_mapped = arch_memory.mapPage(new_vpn, new_ppn, true);
  arch_memory.lock_.release();
  if (!page_mapped)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize: could not map new page\n");
    PageManager::instance()->freePPN(new_ppn);
    return -1;
  }

  return 0;
}

size_t UserSpaceMemoryManager::getTopOfThisPage(size_t address) 
{
  size_t top_stack = address - address%PAGE_SIZE + PAGE_SIZE - sizeof(size_t); 
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

size_t UserSpaceMemoryManager::checkGuardValid(size_t top_current_page)
{
  ArchMemory* arch_memory = &((UserThread*) currentThread)->process_->loader_->arch_memory_;
  size_t top_last_page = top_current_page + PAGE_SIZE;
  for (size_t i = 0; i < MAX_STACK_AMOUNT; i++)
  {
    // check if page is mapped or within user space
    if (top_last_page > USER_BREAK || !arch_memory->checkAddressValid(top_last_page))
    {
      break;
    }
    
    // check if guards are valid
    if (top_last_page && *(size_t*) top_last_page == GUARD_MARKER)
    {
      size_t* guard2 = (size_t*) (top_last_page - sizeof(size_t)*5);
      if (*guard2 == GUARD_MARKER)
      {
        return top_last_page;
      }
      break;
    }
    top_last_page += PAGE_SIZE;
  }
  debug(GROW_STACK, "UserSpaceMemoryManager::checkGuardValid: guards are corrupted. exiting\n");
  return 11;
}