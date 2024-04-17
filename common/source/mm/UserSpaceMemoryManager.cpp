#include "UserSpaceMemoryManager.h"
#include "debug.h"
#include "PageManager.h"
#include "ArchMemory.h"
#include "Loader.h"
#include "UserThread.h"

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


int UserSpaceMemoryManager::checkValidGrowingStack(size_t address)
{
  debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack called with address (%zx)\n", address);
  
  UserThread* current_thread = (UserThread*) currentThread;
  if (!current_thread->guarded_ && )
  {
    /* code */
  }
  
  if(address >= USER_BREAK)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: address is above USER_BREAK\n");
    return 11;
  }
  if(address < current_break_)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: address is below current break\n");
    return 1;
  }
  if(address >= current_break_)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: address is above current break\n");
    return 0;
  }
  return 0;
}


int UserSpaceMemoryManager::increaseStackSize(size_t address)
{
  debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize called with address (%zx)\n", address);
  if(address < current_break_)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize: address is below current break\n");
    return -1;
  }
  lock_.acquire();
  loader_->arch_memory_.lock_.acquire();
  size_t old_break = current_break_;
  size_t new_break = current_break_ + PAGE_SIZE;
  ssize_t size = new_break - current_break_;
  pointer resevered_space = sbrk(size, 1);
  if (resevered_space == 0)
  {
    debug(SBRK, "UserSpaceMemoryManager::increaseStackSize: FATAL ERROR, could not increase stack size at address (%zx)\n", address);
    loader_->arch_memory_.lock_.release();
    lock_.release();
    return -1;
  } 
  else
  {
    debug(SBRK, "UserSpaceMemoryManager::increaseStackSize: stack size is increased successful at address (%zx)\n", current_break_);
    loader_->arch_memory_.lock_.release();
    lock_.release();
    return 0;
  }
}

size_t UserSpaceMemoryManager::getTopOfThisPage() 
{
  size_t stack_variable;
  size_t top_stack = (size_t)&stack_variable - (size_t)(&stack_variable)%PAGE_SIZE + PAGE_SIZE - sizeof(size_t); 
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}
