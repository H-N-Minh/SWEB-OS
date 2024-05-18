#include "Loader.h"
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"


SwappingManager* SwappingManager::instance_ = nullptr;

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


//does only work if the page to swap out is also in the archmemory of the current thread
void SwappingManager::swapOutPage(size_t ppn)
{
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld.\n", ppn);
  assert(ipt_->ipt_lock_.heldBy() == currentThread);
  assert(currentThread->loader_->arch_memory_.archmemory_lock_.heldBy() == currentThread);

  
  if(!ipt_->PPNisInMap(ppn))
  {
    assert(0 && "Try to swap out page that does not exist.");
  }
  
  ustl::vector<VirtualPageInfo*> virtual_page_infos = ipt_->getAndRemoveVirtualPageInfos(ppn);

  //lock disk
  size_t disk_offset = 1; //TODOs (find disk offset)

  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    size_t vpn = virtual_page_info->vpn_;
    if(archmemory != &currentThread->loader_->arch_memory_)
    {
      archmemory->archmemory_lock_.acquire();
    }
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset);
  }

  ipt2_->addVirtualPageInfos(ppn, virtual_page_infos);

  //write to disk
   ArchMemoryMapping m = ArchMemory::resolveMapping(currentThread->loader_->arch_memory_.page_map_level_4_, vpn);
  // (Page*)getIdentAddressOfPPN(m.pt[m.pti].pt.page_ppn);
  // size_t block = NULL;       //target_block_number
  // char* page_data = NULL; //pointer_to_source_data;
  // BDVirtualDevice* bd_device = BDManager::getInstance()->getDeviceByNumber(3);
  // bd_device->writeData(block * bd_device->getBlockSize(), PAGE_SIZE, page_data);
  // unlock disk
 
  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    if(archmemory != &currentThread->loader_->arch_memory_)
    {
      archmemory->archmemory_lock_.release();
    }
  }

  // PageManager::instance()->freePPN(ppn);


  
  }

//Only works if the page i want to swap in is in the archmemory of current thread
int SwappingManager::swapInPage(size_t vpn)
{
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with vpn %ld.\n", vpn);
  ArchMemory& archmemory = currentThread->loader_->arch_memory_; //TODOs Select the right archmemory not nessessary the one of the current thread
  assert(ipt_->ipt_lock_.heldBy() == currentThread);
  assert(archmemory.archmemory_lock_.heldBy() == currentThread);
 

  size_t location_on_disk = archmemory.getDiskLocation(vpn);  //TODOs: at the moment this returns the outdated ppn //and from the current thread arch memory 
  if(!location_on_disk)  //todo: does not really make sense
  {
    assert(0 && "Swapped out page not found");
  }

  //check if there is a free ppn else swap out page and use this ppn

  //read the page from disk

  //check in swapping map which processes and update the values - remove from swapping map

  //add to ipt map

  return 0;
}