#include "Loader.h"
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"

int SwappingManager::disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing = 0;
SwappingManager* SwappingManager::instance_ = nullptr;

SwappingManager::SwappingManager() : disk_lock_("disk_lock_")
{
  assert(!instance_);
  instance_ = this;
   //TODOs needs to be deleted at some point
  ipt_ = new InvertedPageTable();          
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
  assert(ipt_->ipt_lock_.heldBy() == currentThread);
  assert(currentThread->loader_->arch_memory_.archmemory_lock_.heldBy() == currentThread);


  //Find free disk_offset (TODOs)
  size_t disk_offset = disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing;
  disk_offset_should_be_atomic_if_we_do_it_this_way_what_we_probably_not_doing++;

  //Move Page infos from ipt_map_ram to ipt_map_disk
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld to disk offset %ld.\n", ppn, disk_offset);
  ustl::vector<VirtualPageInfo*> virtual_page_infos = ipt_-> moveToOtherMap(ppn, disk_offset, MAPTYPE::IPT_RAM, MAPTYPE::IPT_DISK);

  lock_archmemories_in_right_order(virtual_page_infos);

  ArchMemory* archmemory = NULL;
  size_t vpn = 0;
  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    archmemory = virtual_page_info->arch_memory_;
    vpn = virtual_page_info->vpn_;
  }



  disk_lock_.acquire();
  //write to disk
  ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
  // kprintf("Pagecontent before: <%s>\n", page_content);
  bd_device_->writeData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  disk_lock_.release();

  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    archmemory = virtual_page_info->arch_memory_;
    vpn = virtual_page_info->vpn_;
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset);
  }


  unlock_archmemories(virtual_page_infos);

 
  // PageManager::instance()->freePPN(ppn);
}

//Only works if the page i want to swap in is in the archmemory of current thread
int SwappingManager::swapInPage(size_t vpn)
{
  // debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with vpn %ld.\n", vpn);
  ArchMemory& archmemory = currentThread->loader_->arch_memory_; //TODOs Select the right archmemory not nessessary the one of the current thread
  
  assert(ipt_->ipt_lock_.heldBy() == currentThread);
  assert(archmemory.archmemory_lock_.heldBy() == currentThread);
 
 //Get disk_offset and new ppn
  size_t disk_offset = archmemory.getDiskLocation(vpn);  
  size_t ppn = PageManager::instance()->allocPPN(PAGE_SIZE);

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %ld to ppn %ld.\n", disk_offset, ppn);
  ustl::vector<VirtualPageInfo*> virtual_page_infos = ipt_->moveToOtherMap(disk_offset, ppn, MAPTYPE::IPT_DISK, MAPTYPE::IPT_RAM);



  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    size_t vpn = virtual_page_info->vpn_;
    archmemory->updatePageTableEntryForSwapIn(vpn, ppn);
  }

  disk_lock_.acquire();
  // read the page from disk
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(ppn);
  bd_device_->readData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  // kprintf("Pagecontent after: <%s>\n", page_content);

  disk_lock_.release();
  unlock_archmemories(virtual_page_infos);
  return 0;
}



//TODOs: Does not lock them in the right order yet and does not lock current thread
void SwappingManager::lock_archmemories_in_right_order(ustl::vector<VirtualPageInfo*> virtual_page_infos)
{
  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    if(archmemory != &currentThread->loader_->arch_memory_)
    {
      archmemory->archmemory_lock_.acquire();
    }
  }
}


//TODOs: Does at the moment not unlock the current thread
void SwappingManager::unlock_archmemories(ustl::vector<VirtualPageInfo*> virtual_page_infos)
{
  for(VirtualPageInfo* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->arch_memory_;
    if(archmemory != &currentThread->loader_->arch_memory_)
    {
      archmemory->archmemory_lock_.release();
    }
  }
}