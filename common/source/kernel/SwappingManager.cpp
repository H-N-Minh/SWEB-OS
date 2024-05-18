#include "Loader.h"
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"

int SwappingManager::disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing = 0;
SwappingManager* SwappingManager::instance_ = nullptr;

SwappingManager::SwappingManager()
{
  assert(!instance_);
  instance_ = this;
  ipt_ = new InvertedPageTable();           //TODOs needs to be deleted at some point
  ipt2_ = new InvertedPageTable2();         //TODOs needs to be deleted at some point
  bd_device_ = BDManager::getInstance()->getDeviceByNumber(3);
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

  size_t vpn_of_current_thread = 0;
  //lock disk

  size_t disk_offset = disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing; //TODOs (find free disk offset)
  debug(SWAPPING, "SwappingManager::swapOutPage: disk_offset is %ld.\n", disk_offset);
  disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing++;

  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    size_t vpn = virtual_page_info->vpn_;
    if(archmemory != &currentThread->loader_->arch_memory_)
    {
      archmemory->archmemory_lock_.acquire();
    }
    else
    {
      vpn_of_current_thread = vpn;
    }
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset);
  }
  assert(vpn_of_current_thread && "This only works if current thread wants to swap out");
  ipt2_->addVirtualPageInfos(ppn, virtual_page_infos);

  //write to disk
  ArchMemoryMapping m = ArchMemory::resolveMapping(currentThread->loader_->arch_memory_.page_map_level_4_, vpn_of_current_thread);
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
  int32 rv = bd_device_->writeData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  assert(rv != -1);
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
 

  size_t disk_offset = archmemory.getDiskLocation(vpn);  //TODOs: at the moment this returns the outdated ppn //and from the current thread arch memory 
  debug(SWAPPING, "SwappingManager::swapInPage: disk_offset is %ld.\n", disk_offset);

  size_t ppn = PageManager::instance()->allocPPN(PAGE_SIZE);

  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(ppn);
  bd_device_->readData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  
  archmemory.updatePageTableEntryForSwapIn(vpn, ppn); //should be done for all arch_memories


  //read the page from disk

  //check in swapping map which processes and update the values - remove from swapping map

  //add to ipt map

  return 0;
}