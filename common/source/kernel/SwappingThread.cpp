#include "SwappingThread.h"
#include "Scheduler.h"
#include "SwappingManager.h"
#include "Syscall.h"

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
  if (ipt_initialized_flag_)
  {
    IPTManager* ipt = IPTManager::instance();
    ipt->IPT_lock_.acquire();
    
    // size_t ppn = ipt_manager->findPageToSwapOut();
    // SwappingManager::instance()->swapOutPage(ppn);
    // free_pages_.push_back(ppn);
    debug(MINH, "SwappingThread::updateMetaData: updating meta data\n");
    ipt->IPT_lock_.release();
  }
}

bool SwappingThread::isOneTimeStep()
{
  uint32 current_clock = Syscall::clock();
  debug(MINH, "SwappingThread::isOneTimeStep: current_clock: %d, last_clock_: %d\n", current_clock, last_clock_);
  if (current_clock < last_clock_)    // idk why this happens
  {
    last_clock_ = current_clock;
    return false;
  }
  
  uint32 time_passed = (current_clock - last_clock_) / CLOCKS_PER_SEC;
  if (time_passed >= TIME_STEP)
  {    
    debug(MINH, "SwappingThread::isOneTimeStep: time_passed: %ds\n", time_passed);
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

  ipt_manager->IPT_lock_.release();
}
