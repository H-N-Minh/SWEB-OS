#include "SwappingManager.h"
#include "InvertedPageTable.h"
#include "InvertedPageTable2.h"
#include "UserProcess.h"
#include "UserThread.h"
#include "PageManager.h"

SwappingManager::SwappingManager()
{
  assert(!instance_);
  instance_ = this;
  ipt_ = new InvertedPageTable();           //TODOs needs to be deleted at some point
  ipt2_ = new InvertedPageTable2();         //TODOs needs to be deleted at some point
}

SwappingManager* SwappingManager::instance()
{
  assert(instance_);
  return instance_;
}


void SwappingManager::swapOutPage(size_t ppn)
{
  ipt_->ipt_lock_.acquire();
  ipt2_->ipt2_lock_.acquire();
  
  if(!ipt_->PPNisInMap(ppn))
  {
    assert(0 && "Try to swap out page that does not exist.");
  }
  
  ustl::vector<VirtualPageInfo*> virtual_page_infos = ipt_->getAndRemoveVirtualPageInfos(ppn);

  //lock disk
  size_t disk_offset = 0; //TODOs (find disk offset)

  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    size_t vpn = virtual_page_info->vpn_;
    archmemory->archmemory_lock_.acquire();
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset);
  }

  ipt2_->addVirtualPageInfos(ppn, virtual_page_infos);

  //write to disk

  //unlock disk
 
  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    archmemory->archmemory_lock_.release();
  }

  PageManager::instance()->freePPN(ppn);

  ipt_->ipt_lock_.release();
  ipt2_->ipt2_lock_.release();
}


int SwappingManager::swapInPage(size_t vpn)
{
  ArchMemory& archmemory = currentThread->loader_->arch_memory_; //TODOs Select the right archmemory not nessessary the one of the current thread
  
  ipt_->ipt_lock_.acquire();
  archmemory.archmemory_lock_.acquire();

  size_t location_on_disk = archmemory.getDiskLocation(vpn);  //TODOs: at the moment this returns the outdated ppn //and from the current thread arch memory 
  if(!location_on_disk)
  {
    ipt_->ipt_lock_.release();
    archmemory.archmemory_lock_.release();
    assert(0 && "Swapped out page not found");
  }

  //check if there is a free ppn else swap out page and use this ppn

  //read the page from disk

  //check in swapping map which processes and update the values - remove from swapping map

  //add to ipt map

  
  archmemory.archmemory_lock_.release();
  ipt_->ipt_lock_.release();
  return 0;
}