#include "SwappingThread.h"
#include "Scheduler.h"
#include "SwappingManager.h"
#include "Syscall.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "IPTEntry.h"

#define INVALID_PPN 0

#define TIME_STEP 1 // in seconds
#define TICKS_PER_SEC 18
#define PRESWAP_THRESHOLD 80  // in percentage (start pre-swapping when memory usage is above 80%)
#define SWAP_THRESHOLD 90
#define MAX_PRESWAP_PAGES 20  // maximum total number of pages to pre-swap
#define SWAP_OUT_AMOUNT 10     // max number of pages to swap out at a time
#define SWAP_IN_AMOUNT 10     // max number of pages to swap in at a time

int SwappingThread::user_initialized_flag_ = 0;
bool SwappingThread::should_be_killed_ = false;


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
  // TODO ? lock page manager ? but the lock is private

  uint32_t totalNumPages = pm->getTotalNumPages();
  uint32_t usedNumPages = totalNumPages - pm->getNumFreePages();
  uint32_t usedMemoryRatio = (usedNumPages * 100) / totalNumPages;

  // 80% is the threshold for swapping -> if > threshold then memory is low
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

void SwappingThread::preswap()
{
  if(!SwappingManager::pre_swap_enabled)
    return;

  if (isMemoryAlmostFull() && free_pages_.size() < MAX_PRESWAP_PAGES)
  {
    for (int i = 0; i < SWAP_OUT_AMOUNT; i++)
    {
      if (free_pages_.size() >= MAX_PRESWAP_PAGES)
      {
        break;
      }
      IPTManager* ipt = IPTManager::instance();
      ipt->IPT_lock_.acquire();

      if(!ipt->ram_map_.empty())
      {

        size_t ppn = copyPageToDisk();
        ipt->IPT_lock_.release();
        pre_swapped_pages_.push_back(ppn);
        miss_count_++;
        swap_out_cond_.signal();
      }
      else
      {
        ipt->IPT_lock_.release();
      }
    }
  }
}


void SwappingThread::swapOut()
{
  debug(SWAPTHREAD, "SwappingThread::swapOut: Check if swap out necessary.\n");
  swap_out_lock_.acquire();

  bool memory_full = isMemoryFull();
  if (memory_full && free_pages_.size() < MAX_PRESWAP_PAGES)
  {
    // Swap out multiple pages at a time, until either the vector is full or max swap out amount is reached
    for (int i = 0; i < SWAP_OUT_AMOUNT; i++)
    {
      if (free_pages_.size() >= MAX_PRESWAP_PAGES)
      {
        break;
      }
      IPTManager* ipt = IPTManager::instance();
      ipt->IPT_lock_.acquire();

      if(!ipt->ram_map_.empty())
      {
        size_t ppn = swapPageOut();
        ipt->IPT_lock_.release();
        free_pages_.push_back(ppn);
        miss_count_++;
        swap_out_cond_.signal();
      }
      else
      {
        ipt->IPT_lock_.release();
      }

    }
  }

  if(!memory_full) { // if memory isn't full
    // Free the normal swap out pages
    if (!free_pages_.empty())
    {
      for (uint32 ppn : free_pages_)
      {
        PageManager::instance()->freePPN(ppn);
        debug(MINH, "SwappingThread::swapOut: free page %u\n", ppn);
      }
      free_pages_.clear();
    }

    // And if pre-swap is enabled, free the pre-swapped pages
    if(SwappingManager::pre_swap_enabled && !pre_swapped_pages_.empty()){
      for (uint32 ppn : pre_swapped_pages_)
      {
        PageManager::instance()->freePPN(ppn);
        debug(MINH, "SwappingThread::swapOut: free preswapped page %u\n", ppn);
      }
      pre_swapped_pages_.clear();
    }
    memory_full_try_alloc_again_ = true;
    swap_out_cond_.signal();
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
          it2 = swap_in_map_.erase(it2);
        }
        else
        {
          ++it2;
        }
      }
    }
    //updateMetaData();        //TODOs: adding this makes pra4 work !!!!!!!(maybe bad though) ?????
    swap_in_cond_.broadcast();
  }
  swap_in_lock_.release();
}

void SwappingThread::Run()
{
  while (1)
  {
    if (user_initialized_flag_)
    {
      if (should_be_killed_ && swap_in_map_.empty())   //should be locked
      {
        swap_out_lock_.acquire();
        while(!free_pages_.empty())
        {
          size_t ppn = getFreePage();
          PageManager::instance()->freePPN(ppn);
        }
        swap_out_lock_.release();

        SwappingManager::instance()->swapping_thread_finished_lock_.acquire();
        user_initialized_flag_ = false;
        SwappingManager::instance()->swapping_thread_finished_.signal();
        SwappingManager::instance()->swapping_thread_finished_lock_.release();
        continue;
      }
      // 1. Updating Meta data every 1 seconds
      if (isOneTimeStep())
      {
        updateMetaData();
      }
      // 2. Pre-swap if needed
      preswap();

      // 3. Swap out if needed
      swapOut();
      
      // 4. Swap in if needed
      swapIn();
    }

    // 5. Yield
    Scheduler::instance()->yield();
  }
}


void SwappingThread::updateMetaData()
{
  if (!user_initialized_flag_)
  {
    return;
  }

  IPTManager* ipt = IPTManager::instance();
  if(ipt->pra_type_ == PRA_TYPE::NFU)
  {
    ipt->IPT_lock_.acquire();
    debug(SWAPTHREAD, "SwappingThread::updateMetaData: updating meta data for PRA NFU\n");

    // go through all archmem of each page and check if page was accessed
    for (const auto& pair : ipt->ram_map_)
    {
      IPTEntry* ipt_entry = pair.second;
      assert(ipt_entry && "SwappingThread::updateMetaData: ipt_entry is nullptr\n");
      ustl::vector<ArchmemIPT*>& archmem_vector = ipt_entry->getArchmemIPTs();
      assert(archmem_vector.size() > 0 && "SwappingThread::updateMetaData: key %zu is mapped to no archmem in ram_map_\n");

      bool hit = false;
      // reset the accessed bit for all archmem of same ppn
      for (ArchmemIPT* entry : archmem_vector)
      {
        ArchMemory* archmem = entry->archmem_;
        size_t vpn = entry->vpn_;

        assert(archmem && "SwappingThread::updateMetaData: archmem is nullptr\n");
        archmem->archmemory_lock_.acquire();

        if(archmem->isBitSet(vpn, BitType::ACCESSED, true))
        {
          // Page was accessed, reset the bits
          archmem->resetAccessBits(vpn);
          // debug(SWAPTHREAD, "SwappingThread::updateMetaData: page %zu was accessed. Counter: %d\n", key, ipt->swap_meta_data_[key]);
          hit = true;
        }
        archmem->archmemory_lock_.release();
      }
      if (hit)
      {
        ipt_entry->access_counter_++;
        hit_count_++;
      }
    }
    ipt->IPT_lock_.release();
  }

}

bool SwappingThread::isOneTimeStep()
{
  size_t current_ticks = Scheduler::instance()->getTicks();
  assert(current_ticks >= last_tick_ && "SwappingThread::isOneTimeStep: current_ticks must be greater than last_tick_\n");
  // size_t time_passed = (current_ticks - last_tick_) / TICKS_PER_SEC;
  // if (time_passed >= TIME_STEP)
  // {
  //   // debug(MINH, "SwappingThread::isOneTimeStep: time_passed: %ds\n", time_passed);
  //   last_tick_ = current_ticks;
  //   return true;
  // }
  if((current_ticks - last_tick_)>5)
  {
    return true;
  }
  return false;
}

ustl::deque<size_t> SwappingThread::preswap_page_queue;

size_t SwappingThread::swapPageOut()
{
  IPTManager* ipt = IPTManager::instance();
  size_t ppn_to_swap = INVALID_PPN;
  if (SwappingManager::pre_swap_enabled && !preswap_page_queue.empty())
  {
    ppn_to_swap = preswap_page_queue.front();
    preswap_page_queue.pop_front();
    SwappingManager::instance()->swapOutPage(ppn_to_swap);
  }
  else
  {
    ppn_to_swap = ipt->findPageToSwapOut();
    SwappingManager::instance()->swapOutPage(ppn_to_swap);
  }

  return ppn_to_swap;
}

size_t SwappingThread::copyPageToDisk()
{
  IPTManager* ipt = IPTManager::instance();
  size_t ppn = ipt->findPageToSwapOut();
  preswap_page_queue.push_back(ppn);
  SwappingManager::instance()->copyPageToDisk(ppn);

  return ppn;
}


uint32 SwappingThread::getHitCount()
{
  IPTManager* ipt_manager = IPTManager::instance();
  ipt_manager->IPT_lock_.acquire();
  uint32 hit_count = hit_count_;
  ipt_manager->IPT_lock_.release();
  return hit_count;
}

uint32 SwappingThread::getMissCount()
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

