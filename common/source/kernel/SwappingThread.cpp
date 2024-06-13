#include "SwappingThread.h"
#include "Scheduler.h"
#include "SwappingManager.h"
#include "Syscall.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "IPTEntry.h"


#define TIME_STEP 1 // in seconds
#define TICKS_PER_SEC 18
#define PRESWAP_THRESHOLD 80  // in percentage (start pre-swapping when memory usage is above 80%)
#define SWAP_THRESHOLD 75
#define MAX_PRESWAP_PAGES 20  // maximum total number of pages to pre-swap
#define SWAP_OUT_AMOUNT 10     // max number of pages to swap out at a time
#define SWAP_IN_AMOUNT 10     // max number of pages to swap in at a time

int SwappingThread::user_initialized_flag_ = 0;


SwappingThread::SwappingThread() 
  : Thread(nullptr, "SwappingThread", Thread::KERNEL_THREAD),
    swap_out_lock_("swap_out_lock_"), swap_out_cond_(&swap_out_lock_, "swap_out_cond_"),
    swap_in_lock_("swap_in_lock_"), swap_in_cond_(&swap_in_lock_, "swap_in_cond_")
{
  
}

SwappingThread::~SwappingThread()
{
  assert(false && "SwappingThread destruction means that you probably have accessed an invalid pointer somewhere.");
}

void SwappingThread::kill()
{
  assert(false && "SwappingThread destruction means that you probably have accessed an invalid pointer somewhere.");
}


bool SwappingThread::isMemoryAlmostFull()
{
  PageManager* pm = PageManager::instance();
  uint32_t totalNumPages = pm->getTotalNumPages();
  uint32_t usedNumPages = totalNumPages - pm->getNumFreePages();
  uint32_t usedMemoryRatio = (usedNumPages * 100) / totalNumPages;

  return usedMemoryRatio > PRESWAP_THRESHOLD;
}

bool SwappingThread::isMemoryFull()
{
  PageManager* pm = PageManager::instance();
  uint32_t totalNumPages = pm->getTotalNumPages();
  uint32_t usedNumPages = totalNumPages - pm->getNumFreePages();
  uint32_t usedMemoryRatio = (usedNumPages * 100) / totalNumPages;

  return usedMemoryRatio > SWAP_THRESHOLD;
}

void SwappingThread::preSwap()
{
  if (isMemoryAlmostFull()) {
    IPTManager* ipt = IPTManager::instance();
    ipt->IPT_lock_.acquire();
    size_t ppn = ipt->findPageToSwapOut();
    SwappingManager::instance()->preSwapPage(ppn);
    ipt->IPT_lock_.release();
  }
}

void SwappingThread::swapOut()
{
  swap_out_lock_.acquire();

  bool memory_full = isMemoryFull();
  if (memory_full && free_pages_.size() < MAX_PRESWAP_PAGES)
  {
    for (int i = 0; i < SWAP_OUT_AMOUNT; i++)
    {
      if (free_pages_.size() >= MAX_PRESWAP_PAGES) break;
      size_t ppn = swapPageOut();
      free_pages_.push_back(ppn);
      miss_count_++;
      swap_out_cond_.signal();
    }
  }
  else if (!memory_full)
  {
    if (!free_pages_.empty()) {
      for (uint32 ppn : free_pages_) {
        PageManager::instance()->freePPN(ppn);
      }
      free_pages_.clear();
    }
  }
  swap_out_lock_.release();
}

void SwappingThread::swapIn()
{
  swap_in_lock_.acquire();

  if (!swap_in_map_.empty())
  {
    // Swap in multiple pages at a time, until either no more swap-in requests or max swap-in amount at a time is reached
    for (int i = 0; i < SWAP_IN_AMOUNT; i++)
    {
      if (swap_in_map_.empty())
      {
        break;
      }
      // get the first page in the map and swap it in
      auto it = swap_in_map_.begin();
      size_t disk_offset = it->first;
      ustl::vector<uint32>* preallocated_pages = it->second;

      IPTManager* ipt = IPTManager::instance();
      ipt->IPT_lock_.acquire();
      SwappingManager::instance()->swapInPage(disk_offset, *preallocated_pages);
      ipt->IPT_lock_.release();

      // erase that page and all its values from the map
      for (auto it2 = swap_in_map_.begin(); it2 != swap_in_map_.end();)
      {
        if (it2->first == disk_offset)
        {
          swap_in_map_.erase(it2);
        }
        else
        {
          ++it2;
        }
      }
    }
    swap_in_cond_.broadcast();
  }
  swap_in_lock_.release();
}

[[noreturn]] void SwappingThread::Run()
{
  while (true) {
    if (user_initialized_flag_) {
      if (SwappingManager::pre_swap_enabled && isMemoryAlmostFull())
      {
        preSwap(); // Call preSwap when PRESWAP_THRESHOLD surpassed
      }

      if (isOneTimeStep()) {
        updateMetaData();
      }

      if (isMemoryFull())
      {
        swapOut(); // Call swapOut when SWAP_THRESHOLD surpassed
      }
      swapIn();
    }
    Scheduler::instance()->yield();
  }
}

void SwappingThread::resetAccessedPages(IPTEntry* ipt_entry, bool& hit)
{
  ustl::vector<ArchmemIPT*>& archmem_vector = ipt_entry->getArchmemIPTs();
  assert(!archmem_vector.empty() && "SwappingThread::updateMetaData: key %zu is mapped to no archmem in ram_map_\n");

  // reset the accessed bit for all archmem of same ppn
  for (ArchmemIPT* entry : archmem_vector)
  {
    ArchMemory* archmem = entry->archmem_;
    size_t vpn = entry->vpn_;
    assert(archmem && "SwappingThread::updateMetaData: archmem is nullptr\n");
    archmem->archmemory_lock_.acquire();
    if (archmem->isPageAccessed(vpn))
    {
      // Page was accessed, reset the bits
      archmem->resetAccessDirtyBits(vpn);
      hit = true;
    }
    archmem->archmemory_lock_.release();
  }
}

void SwappingThread::updateMetaData()
{
  if (!user_initialized_flag_)
  {
    return;
  }
  IPTManager* ipt = IPTManager::instance();
  ipt->IPT_lock_.acquire();

  // debug(SWAPTHREAD, "SwappingThread::updateMetaData: updating meta data for PRA NFU\n");
  // go through all archmem of each page and check if page was accessed
  for (const auto& pair : ipt->ram_map_)
  {
    IPTEntry* ipt_entry = pair.second;
    assert(ipt_entry && "SwappingThread::updateMetaData: ipt_entry is nullptr\n");
    bool hit = false;
    resetAccessedPages(ipt_entry, hit);
    if (hit)
    {
      ipt_entry->access_counter_++;
      hit_count_++;
    }
  }
  ipt->IPT_lock_.release();
}

bool SwappingThread::isOneTimeStep()
{
  size_t current_ticks = Scheduler::instance()->getTicks();
  assert(current_ticks >= last_tick_ && "SwappingThread::isOneTimeStep: current_ticks must be greater than last_tick_\n");
  size_t time_passed = (current_ticks - last_tick_) / TICKS_PER_SEC;
  if (time_passed >= TIME_STEP)
  {    
    // debug(MINH, "SwappingThread::isOneTimeStep: time_passed: %ds\n", time_passed);
    last_tick_ = current_ticks;
    return true;
  }
  return false;
}

size_t SwappingThread::swapPageOut()
{
  IPTManager* ipt = IPTManager::instance();
  ipt->IPT_lock_.acquire();
  size_t ppn = ipt->findPageToSwapOut();
  SwappingManager::instance()->swapOutPage(ppn);
  ipt->IPT_lock_.release();
  return ppn;
}

uint32 SwappingThread::getHitCount() const
{
  IPTManager* ipt_manager = IPTManager::instance();
  ipt_manager->IPT_lock_.acquire();
  uint32 hit_count = hit_count_;
  ipt_manager->IPT_lock_.release();
  return hit_count;
}

uint32 SwappingThread::getMissCount() const
{
  IPTManager* ipt_manager = IPTManager::instance();
  ipt_manager->IPT_lock_.acquire();
  uint32 miss_count = miss_count_;
  ipt_manager->IPT_lock_.release();
  return miss_count;
}

void SwappingThread::addSwapIn(size_t disk_offset, ustl::vector<uint32>* preallocated_pages)
{
  assert(swap_in_lock_.isHeldBy((Thread*) currentThread) && "SwappingThread::addSwapIn: swap_in_lock_ must be held by currentThread\n");
  swap_in_map_.insert({disk_offset, preallocated_pages});
}

bool SwappingThread::isOffsetInMap(size_t disk_offset)
{
  assert(swap_in_lock_.isHeldBy((Thread*) currentThread) && "SwappingThread::isOffsetInMap: swap_in_lock_ must be held by currentThread\n");
  return swap_in_map_.find(disk_offset) != swap_in_map_.end();
}

bool SwappingThread::isFreePageAvailable()
{
  assert(swap_out_lock_.isHeldBy((Thread*) currentThread) && "SwappingThread::isFreePageAvailable: swap_out_lock_ must be held by currentThread\n");
  return !free_pages_.empty();
}

uint32 SwappingThread::getFreePage()
{
  assert(swap_out_lock_.isHeldBy((Thread*) currentThread) && "SwappingThread::getFreePage: swap_out_lock_ must be held by currentThread\n");
  assert(!free_pages_.empty() && "SwappingThread::getFreePage: free_pages_ is empty\n");
  uint32 ppn = free_pages_.front();
  free_pages_.erase(free_pages_.begin());
  return ppn;
}