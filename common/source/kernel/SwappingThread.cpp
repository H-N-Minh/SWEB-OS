#include "SwappingThread.h"
#include "Scheduler.h"
#include "SwappingManager.h"
#include "Syscall.h"
#include "ArchMemory.h"

#define TIME_STEP 2 // in seconds
#define CLOCKS_PER_SEC 1000000

int SwappingThread::ipt_initialized_flag_ = 0;


SwappingThread::SwappingThread() 
  : Thread(0, "SwappingThread", Thread::KERNEL_THREAD), orders_lock_("swapping_orders_lock_"),
    orders_cond_(&orders_lock_, "swapping_orders_cond_"), orders_(0), last_clock_(0)
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

void SwappingThread::Run()
{
  while (1)
  {
    // debug(MINH, "SwappingThread::Run: running\n");
    orders_lock_.acquire();
    assert(orders_ >= 0 && "SwappingThread::Run: number of orders is negative\n");

    if (isOneTimeStep())
    {
      updateMetaData();
    }
    
    if (orders_ == 0)
    {
      orders_lock_.release();
      Scheduler::instance()->yield();
    }
    else
    {
      assert(orders_ > 0 && "SwappingThread::Run: number of orders is <= 0\n");
      swapPageOut();
      orders_--;
      orders_cond_.signal();
      orders_lock_.release();
    }
  }
}

void SwappingThread::updateMetaData()
{
  assert(orders_lock_.isHeldBy((Thread*) currentThread) && "SwappingThread::updateMetaData: orders_lock_ is not held by currentThread\n");
 
  if (!ipt_initialized_flag_)
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
      ustl::vector<IPTEntry*> entries = ipt->getRamEntriesFromKey(key);
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
  uint32 current_clock = Syscall::clock();
  // debug(MINH, "SwappingThread::isOneTimeStep: current_clock: %d, last_clock_: %d\n", current_clock, last_clock_);
  if (current_clock < last_clock_)    // idk why this happens
  {
    last_clock_ = current_clock;
    return false;
  }
  
  uint32 time_passed = (current_clock - last_clock_) / CLOCKS_PER_SEC;
  if (time_passed >= TIME_STEP)
  {    
    // debug(MINH, "SwappingThread::isOneTimeStep: time_passed: %ds\n", time_passed);
    last_clock_ = current_clock;
    return true;
  }
  return false;
}

void SwappingThread::swapPageOut()
{
  IPTManager* ipt_manager = IPTManager::instance();
  ipt_manager->IPT_lock_.acquire();
  
  size_t ppn = ipt_manager->findPageToSwapOut();
  SwappingManager::instance()->swapOutPage(ppn);
  free_pages_.push_back(ppn);
  miss_count_++;

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