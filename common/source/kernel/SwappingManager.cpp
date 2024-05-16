#include "SwappingManager.h"
#include "InvertedPageTable.h"
#include "UserProcess.h"
#include "UserThread.h"

SwappingManager::SwappingManager()
{
  assert(!instance_);
  instance_ = this;
  ipt_ = new InvertedPageTable();
}

SwappingManager* SwappingManager::instance()
{
  assert(instance_);
  return instance_;
}


void SwappingManager::swapOutPage()
{
  ipt_->ipt_lock_.acquire();

  ipt_->ipt_lock_.release();
}


int SwappingManager::swapInPage(size_t vpn)
{
  ipt_->ipt_lock_.acquire();
  size_t location_on_disk = currentThread->loader_->arch_memory_.getDiskLocation(vpn);  //TODOs: at the moment this returns the outdated ppn
  if(!location_on_disk)
  {
    ipt_->ipt_lock_.release();
    assert(0 && "Swapped out page not found");
  }

  //read the page from disk

  
  ipt_->ipt_lock_.release();
  return 0;
}