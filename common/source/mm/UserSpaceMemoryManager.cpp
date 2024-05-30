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
  heap_start_ = (size_t) loader->getBrkStart();
  current_break_ = heap_start_;
  loader_ = loader;
}


void* UserSpaceMemoryManager::sbrk(ssize_t size)
{
  debug(SBRK, "UserSpaceMemoryManager::sbrk called with size (%zd).\n", size);

  assert(current_break_lock_.heldBy() == currentThread && "Currentbreak needs to be locked");
  size_t potential_new_break = current_break_ + size;
  if (potential_new_break > MAX_HEAP_SIZE || potential_new_break < heap_start_)
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
        if (loader_->arch_memory_.checkAddressValid(old_top_vpn))
        {
          loader_->arch_memory_.unmapPage(old_top_vpn);
        }
        loader_->arch_memory_.archmemory_lock_.release();
        IPTManager::instance()->IPT_lock_.release();
        old_top_vpn--;
      }
    }
    
    assert(current_break_ >= heap_start_ && "UserSpaceMemoryManager::sbrk: current break is below heap start");
    assert(current_break_ <= MAX_HEAP_SIZE && "UserSpaceMemoryManager::sbrk: current break is above heap limit");
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

  if (new_break_addr > MAX_HEAP_SIZE || new_break_addr < heap_start_)
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
  if (address <= MAX_HEAP_SIZE)
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
  
  // TODO MINH: growing stack  now has broken locking because of new allocPPN rule
  uint64 new_vpn = (top_this_page + sizeof(size_t)) / PAGE_SIZE - 1;
  uint32 new_ppn = PageManager::instance()->allocPPN(); //TODO MINH: this alloc and the prealloc below should be put outside locks
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


////////////////////////////////////////////////////////////////




size_t UserSpaceMemoryManager::bytesNeededForMemoryBlock(size_t size)
{
  return size + sizeof(MemoryBlock) + sizeof(char);
}

int UserSpaceMemoryManager::allocateMemoryWithSbrk(size_t bytes_needed)
{
  size_t pages_needed = (bytes_needed / (PAGE_SIZE + 1)) + 1;
  free_bytes_left_on_page_ =  PAGE_SIZE - (bytes_needed % PAGE_SIZE);

  if(free_bytes_left_on_page_ == PAGE_SIZE)
  {
    free_bytes_left_on_page_ = 0;
  }

  void* rv = (void*)sbrk((ssize_t)(pages_needed * PAGE_SIZE));
  if(rv == (void*)-1)
  {
    return -1;
  }
  return 0;
}

void UserSpaceMemoryManager::createNewMemoryBlock(MemoryBlock* memory_block, size_t size, bool is_free, void* address, MemoryBlock* next)
{
  memory_block->size_ = size;
  memory_block->is_free_ = is_free;
  memory_block->address_ = address;
  memory_block->next_ = next;
}

void UserSpaceMemoryManager::addOverflowProtection(MemoryBlock* memory_block)
{
  *((char*)((size_t)memory_block->address_ + memory_block->size_)) = '|';
}

bool UserSpaceMemoryManager::checkOverflowProtection(MemoryBlock* memory_block)
{
  if(*((char*)((size_t)memory_block->address_ + memory_block->size_)) == '|')
  {
    return true;
  }
  else
  {
    return false;
  }
}

size_t free_bytes_left_on_page_ = 0;

MemoryBlock* first_memory_block_ = NULL;
MemoryBlock* heap_start__;


void* UserSpaceMemoryManager::malloc(size_t size)
{
  if(size == 0)
  {
    return NULL;
  }

  current_break_lock_.acquire();
  if(first_malloc_call) //TODOs make check atomic
  {
    heap_start__ = (MemoryBlock*)sbrk(0);
    first_malloc_call = false;
  }

  if(!first_memory_block_)
  {
    size_t bytes_needed = bytesNeededForMemoryBlock(size);
    int rv = allocateMemoryWithSbrk(bytes_needed);
    if(rv == -1)
    {
      current_break_lock_.release();
      return NULL;
    }

    first_memory_block_ = heap_start__;
    createNewMemoryBlock(first_memory_block_, size, false, first_memory_block_ + 1, NULL);
    addOverflowProtection(first_memory_block_);
    used_block_counts_++;
    current_break_lock_.release();
    return first_memory_block_->address_;
  }
  else
  {
    MemoryBlock* next_memory_block = first_memory_block_;
    while(1)
    {
      if(!checkOverflowProtection(next_memory_block))
      {
        current_break_lock_.release();
        Syscall::exit(-1);
      }
      if(next_memory_block->is_free_ && next_memory_block->size_ >= size)
      {
        if(next_memory_block->size_ >= bytesNeededForMemoryBlock(size) + bytesNeededForMemoryBlock(0))
        {
          MemoryBlock* new_unused_memory_block = (MemoryBlock*)((size_t)next_memory_block + bytesNeededForMemoryBlock(size));
          createNewMemoryBlock(new_unused_memory_block, next_memory_block->size_ - bytesNeededForMemoryBlock(size), true, new_unused_memory_block + 1, next_memory_block->next_);

          next_memory_block->next_ = new_unused_memory_block;
          next_memory_block->size_ = size;
          addOverflowProtection(next_memory_block);

          if(new_unused_memory_block->next_ && new_unused_memory_block->next_->is_free_)
          {
            new_unused_memory_block->size_ = new_unused_memory_block->size_ + bytesNeededForMemoryBlock(new_unused_memory_block->next_->size_);
            new_unused_memory_block->next_ = new_unused_memory_block->next_->next_;
          }
        }
        next_memory_block->is_free_ = false;
        used_block_counts_++;
        current_break_lock_.release();
        return next_memory_block->address_;
      }
      //last element of linked list reached
      else if(next_memory_block->next_ == NULL)
      {
        if(bytesNeededForMemoryBlock(size) <= free_bytes_left_on_page_)
        {
          free_bytes_left_on_page_ -=  bytesNeededForMemoryBlock(size);
        }
        else
        {
          size_t bytes_needed = bytesNeededForMemoryBlock(size) - free_bytes_left_on_page_;
          int rv = allocateMemoryWithSbrk(bytes_needed);
          if(rv == -1)
          {
            current_break_lock_.release();
            return NULL;
          }
        }
        MemoryBlock* memory_block_new = (MemoryBlock*)((size_t)next_memory_block + bytesNeededForMemoryBlock(next_memory_block->size_));
        createNewMemoryBlock(memory_block_new, size, false, memory_block_new + 1, NULL);
        addOverflowProtection(memory_block_new);
        used_block_counts_++;
        next_memory_block->next_ = memory_block_new;
        
        current_break_lock_.release();
        return memory_block_new->address_;
      }
      else
      {
        next_memory_block = next_memory_block->next_;
      }
    }
    current_break_lock_.release();
  }
}


void UserSpaceMemoryManager::free(void *ptr)
{
  if(ptr == NULL) //TODOs (check pointer)
  {
    return;
  }

  current_break_lock_.acquire();
  MemoryBlock* element_to_free = (MemoryBlock*)ptr - 1;
  MemoryBlock* next = first_memory_block_;
  if(next == NULL)
  {
    current_break_lock_.release();
    Syscall::exit(-1);
  }
  MemoryBlock* element_before;
  while(next->next_ != NULL)
  {
    if(*((char*)((size_t)next->address_ + next->size_)) != '|')
    {
      current_break_lock_.release();
      Syscall::exit(-1);
    }
    if(next->next_ == element_to_free)
    {
      element_before = next;
    }
    next = next->next_;
  }
  if(!element_before && element_to_free != first_memory_block_) //Check if element can be found in list
  {
    current_break_lock_.release();
    Syscall::exit(-1);
  }
  if(element_to_free->is_free_) //check if element is already free
  {
    current_break_lock_.release();
    Syscall::exit(-1);
  }
  used_block_counts_--;
  if(used_block_counts_ == 0)
  {
    free_bytes_left_on_page_ = 0;
    int rv = brk((size_t)first_memory_block_);
    if(rv != 0)
    {
      current_break_lock_.release();
      Syscall::exit(-1);
    }
    first_memory_block_ = NULL;
    current_break_lock_.release();
    return;
  }
  element_to_free->is_free_ = true;

  if(element_to_free->next_ && element_to_free->next_->is_free_)
  {
    element_to_free->size_ = element_to_free->size_ + element_to_free->next_->size_ + sizeof(MemoryBlock) + sizeof(char);
    element_to_free->next_ = element_to_free->next_->next_;
  }
  if(element_to_free != first_memory_block_ &&element_before->is_free_)
  {
    element_before->size_ = element_before->size_ + element_before->next_->size_ + sizeof(MemoryBlock) + sizeof(char);
    element_before->next_ = element_before->next_->next_;
  }
  current_break_lock_.release();
}


void* UserSpaceMemoryManager::calloc(size_t num_memb, size_t size_each)
{
  void* temp = malloc(num_memb * size_each);
  memset(temp, 0, num_memb * size_each);
  return temp; 
}



