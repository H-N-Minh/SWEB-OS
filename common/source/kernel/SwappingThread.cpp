#include "SwappingThread.h"
#include "Scheduler.h"

SwappingThread::SwappingThread() 
  : Thread(0, "SwappingThread", Thread::KERNEL_THREAD), orders_lock_("swapping_orders_lock_"),
    orders_cond_(&orders_lock_, "swapping_orders_cond_"), orders_(0)
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

    if (orders_ == 0)
    {
      orders_lock_.release();
      Scheduler::instance()->yield();
    }
    else
    {
      orders_--;
      orders_cond_.signal();
      orders_lock_.release();
    }
    
  }
}

