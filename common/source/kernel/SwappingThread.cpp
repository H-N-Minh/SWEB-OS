#include "SwappingThread.h"
#include "Scheduler.h"
#include "SwappingManager.h"
#include "Syscall.h"
#include "ArchMemory.h"
#include "PageManager.h"

#define TIME_STEP 2 // in seconds
#define TICKS_PER_SEC 18
#define PRESWAP_THRESHOLD 80  // in percentage (start pre-swapping when memory usage is above 80%)
#define MAX_PRESWAP_PAGES 20  // maximum total number of pages to pre-swap
#define SWAP_OUT_AMOUNT 10     // max number of pages to swap out at a time
#define SWAP_IN_AMOUNT 10     // max number of pages to swap in at a time

int SwappingThread::user_initialized_flag_ = 0;
bool SwappingThread::should_be_killed_ = false;


SwappingThread::SwappingThread() 
  : Thread(0, "SwappingThread", Thread::KERNEL_THREAD), 
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


bool SwappingThread::isTimeToSwapOut()
{
  PageManager* pm = PageManager::instance();
  // TODO ? lock pagemanager ? but the lock is private

  uint32_t totalNumPages = pm->getTotalNumPages();
  uint32_t usedNumPages = totalNumPages - pm->getNumFreePages();
  uint32_t usedMemoryRatio = (usedNumPages * 100) / totalNumPages;

  // 80% is the threshold for swapping -> can be changed
  bool isMemoryNearLimit = usedMemoryRatio > PRESWAP_THRESHOLD;

  // if memory is near limit AND our free_pages_ vector is not full -> swap out
  return isMemoryNearLimit && (free_pages_.size() < MAX_PRESWAP_PAGES);
}

void SwappingThread::swapOut()
{
  swap_out_lock_.acquire();

  if (isTimeToSwapOut())
  {
    // Swap out multiple pages at a time, until either the vector is full or max swap out amount is reached
    for (int i = 0; i < SWAP_OUT_AMOUNT; i++)
    {
      if (free_pages_.size() >= MAX_PRESWAP_PAGES)
      {
        break;
      }
      
      size_t ppn = swapPageOut();
      free_pages_.push_back(ppn);
      miss_count_++;
      swap_out_cond_.signal();
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
      kprintf("1\n");
      if (swap_in_map_.empty())
      {
        kprintf("2\n");
        break;
      }
      kprintf("3\n");
      // get the first page in the map and swap it in
      auto it = swap_in_map_.begin();
      kprintf("4\n");
      size_t disk_offset = it->first;
      kprintf("5\n");
      ustl::vector<uint32>* preallocated_pages = it->second;
      kprintf("6\n");
      IPTManager* ipt = IPTManager::instance();
      kprintf("7\n");
      ipt->IPT_lock_.acquire();
      kprintf("8\n");
      SwappingManager::instance()->swapInPage(disk_offset, *preallocated_pages);
      kprintf("9\n");
      ipt->IPT_lock_.release();

      // erase that page and all its values from the map
      for (auto it2 = swap_in_map_.begin(); it2 != swap_in_map_.end();)             //TODOs: this loop previously get stuck - not sure if my solution is correct
      {
        if (it2->first == disk_offset)
        {
          swap_in_map_.erase(it2);
          break;
        }
        else
        {
          ++it;
        }
      }
    }
    swap_in_cond_.broadcast();
    kprintf("10\n");
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
      // 1. Updating Meta data for PRA NFU every 2 seconds
      if (isOneTimeStep())
      {
        updateMetaData();
      }

      // 2. Swap out if needed
      swapOut();
      
      // 3. Swap in if needed
      swapIn();
    }


    // 4. Yield
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
  ipt->IPT_lock_.acquire();

  if (ipt->pra_type_ == NFU)
  {
    // debug(SWAPTHREAD, "SwappingThread::updateMetaData: updating meta data for PRA NFU\n");
    // check validity of swap_meta_data_ and ram_map_
    ipt->checkSwapMetaDataConsistency();

    // go through all archmem of each page and check if page was accessed
    for (const auto& pair : ipt->swap_meta_data_)
    {
      size_t key = (size_t) pair.first;
      ustl::vector<IPTEntry*> entries = ipt->getIptEntriesFromKey(key, IPTMapType::RAM_MAP);
      assert(entries.size() > 0 && "SwappingThread::updateMetaData: key %zu is mapped to no IPTEntries in ram_map_\n");
      
      for (IPTEntry* entry : entries)
      {
        ArchMemory* archmem = entry->archmem_;
        assert(archmem && "SwappingThread::updateMetaData: archmem is nullptr\n");
        archmem->archmemory_lock_.acquire();

        if (entry->archmem_->isPageAccessed(entry->vpn_))
        {
          // Page was accessed, reset the bits, update data then break the loop to check next ppn
          entry->archmem_->resetAccessDirtyBits(entry->vpn_);
          ipt->swap_meta_data_[key]++;
          archmem->archmemory_lock_.release();
          debug(SWAPTHREAD, "SwappingThread::updateMetaData: page %zu was accessed. Counter: %d\n", key, ipt->swap_meta_data_[key]);
          hit_count_++;
          break;
        }
        archmem->archmemory_lock_.release();
      }
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

void SwappingThread::swap10PagesOut()
{
  IPTManager* ipt_manager = IPTManager::instance();
  ipt_manager->IPT_lock_.acquire();

  for(int i=0; i<10; i++)
  {
    if(free_pages_.size() >= 20)
    {
      // if free_pages_ already has 20 pages, we break the loop early
      break;
    }
    bool thereAnyPageToSwapOut = ipt_manager->isThereAnyPageToSwapOut();
    if (thereAnyPageToSwapOut)
    {
      size_t ppn = ipt_manager->findPageToSwapOut();
      SwappingManager::instance()->swapOutPage(ppn);
      free_pages_.push_back(ppn);
      miss_count_++;
    }
  }

  ipt_manager->IPT_lock_.release();
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