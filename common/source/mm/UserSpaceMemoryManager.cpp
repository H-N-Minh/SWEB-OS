#include "UserSpaceMemoryManager.h"
#include "debug.h"
#include "PageManager.h"
#include "ArchMemory.h"
#include "Loader.h"
#include "UserProcess.h"
#include "ArchMemory.h"


size_t UserSpaceMemoryManager::totalUsedHeap()
{
  return current_break_ - heap_start_;
}

UserSpaceMemoryManager::UserSpaceMemoryManager(Loader* loader)
  : current_break_lock_("UserSpaceMemoryManager::lock_")
{
  heap_start_ = 0x800000000;
  current_break_ = heap_start_;
  loader_ = loader;
  shared_mem_manager_ = new SharedMemManager();

}

UserSpaceMemoryManager::UserSpaceMemoryManager(const UserSpaceMemoryManager& other, Loader* loader)
  : current_break_(other.current_break_), heap_start_(other.heap_start_), loader_(loader),
    current_break_lock_("UserSpaceMemoryManager::lock_")
{
  shared_mem_manager_ = new SharedMemManager(*other.shared_mem_manager_);
  loader->arch_memory_.shared_mem_manager_ = shared_mem_manager_;
}


UserSpaceMemoryManager::~UserSpaceMemoryManager()
{
  delete shared_mem_manager_;
}


void* UserSpaceMemoryManager::sbrk(ssize_t size)
{
  debug(SBRK, "UserSpaceMemoryManager::sbrk called with size (%zd).\n", size);

  assert(current_break_lock_.heldBy() == currentThread && "Currentbreak needs to be locked");
  size_t potential_new_break = current_break_ + size;
  if (potential_new_break > MAX_HEAP_ADDRESS || potential_new_break < heap_start_)
  {
    debug(SBRK, "Syscall::sbrk: size %zd is too big\n", size);
    return (void*)-1;
  }


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

    if(size < 0)
    {
      debug(SBRK, "old break is on page %zx >= new break is on page %zx\n", old_top_vpn, new_top_vpn);
      while(old_top_vpn != new_top_vpn)
      {
        IPTManager::instance()->IPT_lock_.acquire();
        loader_->arch_memory_.archmemory_lock_.acquire();
        if (loader_->arch_memory_.checkAddressValid(old_top_vpn * PAGE_SIZE))  //TODOs not sure if this is correct
        {
          loader_->arch_memory_.unmapPage(old_top_vpn);
        }
        loader_->arch_memory_.archmemory_lock_.release();
        IPTManager::instance()->IPT_lock_.release();
        old_top_vpn--;
      }
    }
    
    assert(current_break_ >= heap_start_ && "UserSpaceMemoryManager::sbrk: current break is below heap start");
    assert(current_break_ <= MAX_HEAP_ADDRESS && "UserSpaceMemoryManager::sbrk: current break is above heap limit");
    debug(SBRK, "UserSpaceMemoryManager::sbrk: break is changed successful, new break value is %zx\n", current_break_);
    return (void*)old_break;
  }
  else
  {
    debug(SBRK, "UserSpaceMemoryManager::sbrk: returning current break value without changing it %zx\n", current_break_);
    return (void*)current_break_;
  }
}


int UserSpaceMemoryManager::brk(size_t new_break_addr)
{
  assert(current_break_lock_.heldBy() == currentThread && "Currentbreak needs to be locked");

  debug(SBRK, "UserSpaceMemoryManager::brk called with new break address (%zx)\n", new_break_addr);

  if (new_break_addr > MAX_HEAP_ADDRESS || new_break_addr < heap_start_)
  {
    debug(SBRK, "Syscall::brkMemory: address %p is not within heap segment\n", (void*) new_break_addr);
    return -1;
  }

  ssize_t size = new_break_addr - current_break_;
  void* resevered_space = sbrk(size);
  if (resevered_space == (void*)-1)
  {
    debug(SBRK, "UserSpaceMemoryManager::brk: FATAL ERROR, could not set new break at address (%zx)\n", new_break_addr);
    return -1;
  }
  else
  {
    debug(SBRK, "UserSpaceMemoryManager::brk: new break is set successful at address (%zx)\n", current_break_);
    return 0;
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
  if (address <= MAX_HEAP_ADDRESS)
  {
    return 0;
  }

  // UserThread* current_thread = (UserThread*) currentThread;
  // if (address > current_thread->top_stack_ || address < (current_thread->top_stack_ - MAX_STACK_AMOUNT*PAGE_SIZE + sizeof(size_t)))
  // {
  //   debug(GROW_STACK, "UserSpaceMemoryManager::sanityCheck: address is not in range of growing stack\n");
  //   return 0;
  // }
  ArchMemory* arch_memory = &((UserThread*) currentThread)->process_->loader_->arch_memory_;
  if (arch_memory->checkAddressValid(address))
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::sanityCheck: address is already mapped\n");
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
    return NOT_RELATED_TO_GROWING_STACK;
  }

  // get to top of stack where the meta data is stored
  debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: checking for overflow/underflow corruption\n");
  size_t top_current_stack = getTopOfThisStack(address);
  if (top_current_stack == 0)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: guards are corrupted or not found. Segfault!!\n");
    return NOT_RELATED_TO_GROWING_STACK;
  }

  // check if the guards are intact. this also checks overflow underflow
  int is_guard_valid = checkGuardValid(top_current_stack);
  if (is_guard_valid == 0)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: guards are corrupted or not found. Segfault!!\n");
    return GROWING_STACK_FAILED;
  }

  finalSanityCheck(address, top_current_stack);
  debug(GROW_STACK, "UserSpaceMemoryManager::checkValidGrowingStack: growing stack request is valid\n");

  return GROWING_STACK_VALID;
}


void UserSpaceMemoryManager::finalSanityCheck(size_t address, size_t top_current_stack)
{
  assert(top_current_stack && "top_current_stack pointer of the current stack is NULL");
  // assert(top_current_stack == ((UserThread*) currentThread)->top_stack_ && "this is not our stack");

  assert(address < top_current_stack && "address is not within range of growing stack");
  assert(address > top_current_stack - PAGE_SIZE*MAX_STACK_AMOUNT && "address is not within range of growing stack");

  size_t* guard1 = (size_t*) top_current_stack;
  size_t* guard2 = (size_t*) (top_current_stack - sizeof(size_t)* (META_SIZE - 1));
  assert(guard1 && "guard1 is corrupted");
  assert(guard2 && "guard2 is corrupted");
}


int UserSpaceMemoryManager::increaseStackSize(size_t address)
{
  ArchMemory* arch_memory = &((UserThread*) currentThread)->process_->loader_->arch_memory_;
  assert(arch_memory->archmemory_lock_.heldBy() == currentThread);
  assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread);
  debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize called with address (%zx)\n", address);

  // Quick check to see if the address is (somewhat) valid
  size_t top_this_page = getTopOfThisPage(address);
  size_t top_this_stack = getTopOfThisStack(address);
  assert(top_this_stack != 0 && "UserSpaceMemoryManager::increaseStackSize: guards are corrupted. Segfault!!");
  finalSanityCheck(address, top_this_stack);

  // Set up new page
  debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize: passed sanity check, setting up new page\n");
  
  // TODOMINH: growing stack  now has broken locking because of new allocPPN rule
  uint64 new_vpn = (top_this_page + sizeof(size_t)) / PAGE_SIZE - 1;
  uint32 new_ppn = PageManager::instance()->allocPPN(); //TODOMINH: this alloc and the prealloc below should be put outside locks
  ustl::vector<uint32> preallocated_pages = PageManager::instance()->preAlocatePages(3); // for mapPage later
  bool page_mapped = arch_memory->mapPage(new_vpn, new_ppn, true, preallocated_pages);
  PageManager::instance()->releaseNotNeededPages(preallocated_pages);

  if (!page_mapped)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::increaseStackSize: could not map new page\n");
    PageManager::instance()->freePPN(new_ppn);
    return GROWING_STACK_FAILED;
  }

  return GROWING_STACK_VALID;
}

size_t UserSpaceMemoryManager::getTopOfThisPage(size_t address)
{
  size_t top_stack = address - address%PAGE_SIZE + PAGE_SIZE - sizeof(size_t);
  assert(top_stack && "top_stack pointer of the current stack is NULL somehow, check the calculation");
  return top_stack;
}

size_t UserSpaceMemoryManager::getTopOfThisStack(size_t address)
{
  size_t top_current_stack = getTopOfThisPage(address);
  ArchMemory& arch_memory = ((UserThread*) currentThread)->process_->loader_->arch_memory_;
  for (size_t i = 0; i < MAX_STACK_AMOUNT; i++)
  {
    if (top_current_stack && top_current_stack < USER_BREAK)
    {
      if (arch_memory.checkAddressValid(top_current_stack) && *(size_t*) top_current_stack == GUARD_MARKER)
      {
        return top_current_stack;
      }
    }
    top_current_stack += PAGE_SIZE;
  }
  return 0;
}


int UserSpaceMemoryManager::checkGuardValid(size_t top_current_stack)
{
  // guards of this thread
  size_t guard1 = top_current_stack;
  assert(guard1 && "top_stack_ is uninitialized");
  size_t guard2 = guard1 - sizeof(size_t)*(META_SIZE - 1);
  // guards of the thread beneath
  size_t guard3 = guard1 - PAGE_SIZE * MAX_STACK_AMOUNT;
  size_t guard4 = guard3 - sizeof(size_t)*(META_SIZE - 1);

  debug(GROW_STACK, "UserSpaceMemoryManager::checkGuardValid: checking guards of current thread\n");
  if (*(size_t*) guard1 != GUARD_MARKER || *(size_t*) guard2 != GUARD_MARKER)
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::checkGuardValid: guards of current thread are corrupted\n");
    return 0;
  }
  debug(GROW_STACK, "UserSpaceMemoryManager::checkGuardValid: Guards current thread is intact\n");

  ArchMemory* arch_memory = &((UserThread*) currentThread)->process_->loader_->arch_memory_;


  if (arch_memory->checkAddressValid(guard3))
  {
    debug(GROW_STACK, "UserSpaceMemoryManager::sanityCheck: Theres another thread below us, checking its guards\n");
    if (*(size_t*) guard3 != GUARD_MARKER || *(size_t*) guard4 != GUARD_MARKER)
    {
      debug(GROW_STACK, "UserSpaceMemoryManager::checkGuardValid: Guards of the thread below is corrupted\n");
      return 0;
    }
  }

  debug(GROW_STACK, "UserSpaceMemoryManager::checkGuardValid: All guards are still intact\n");
  return 1;
}



