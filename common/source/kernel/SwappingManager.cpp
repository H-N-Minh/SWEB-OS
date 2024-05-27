#include "Loader.h" //
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"

size_t SwappingManager::disk_offset_ = 0;
SwappingManager* SwappingManager::instance_ = nullptr;

SwappingManager::SwappingManager() : disk_lock_("disk_lock_")
{
  assert(!instance_);
  instance_ = this;
   //TODOs needs to be deleted at some point
  ipt_ = new IPTManager();          
  bd_device_ = BDManager::getInstance()->getDeviceByNumber(3);
  bd_device_->setBlockSize(PAGE_SIZE);
  debug(SWAPPING, "Blocksize %d.\n", bd_device_->getBlockSize());
}

SwappingManager* SwappingManager::instance()
{
  return instance_;
}

SwappingManager::~SwappingManager()
{
  delete ipt_;
}


//does only work if the page to swap out is also in the archmemory of the current thread
void SwappingManager::swapOutPage(size_t ppn)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  //Move Page infos from ipt_map_ram to ipt_map_disk
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld to disk offset %ld.\n", ppn, disk_offset_);
  ustl::vector<IPTEntry*> virtual_page_infos = ipt_-> moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset_);

  // lock all archmem and choose 1 archmem and vpn (for later to write to disk)
  lock_archmemories_in_right_order(virtual_page_infos);

  // for debuging
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem;
    size_t vpn = virtual_page_info->vpn;
    debug(SWAPPING, "SwappingManager::swapOutPage: vpn: %ld, archmemory: %p (ppn %ld -> disk offset %ld).\n", vpn, archmemory, ppn, disk_offset_);
  }

  ArchMemory* archmemory = virtual_page_infos[0]->archmem;
  size_t vpn = virtual_page_infos[0]->vpn;

  //write to disk
  disk_lock_.acquire();
  ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
  // kprintf("Pagecontent before: <%s>\n", page_content);
  bd_device_->writeData(disk_offset_ * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  disk_lock_.release();
  total_disk_writes_++;

  // updating all archmem to point to disk
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    archmemory = virtual_page_info->archmem;
    vpn = virtual_page_info->vpn;
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset_);
  }
  disk_offset_++;

  PageManager::instance()->ref_count_lock_.acquire();  //??
  PageManager::instance()->setReferenceCount(ppn, 0);
  PageManager::instance()->ref_count_lock_.release();

  unlock_archmemories(virtual_page_infos);
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld finished", ppn);
}

//Only works if the page i want to swap in is in the archmemory of current thread
int SwappingManager::swapInPage(size_t vpn, ustl::vector<uint32>& preallocated_pages)
{
  ArchMemory& archmemory = currentThread->loader_->arch_memory_; //TODOs Select the right archmemory not nessessary the one of the current thread
  
  assert(ipt_->IPT_lock_.heldBy() == currentThread);
  assert(archmemory.archmemory_lock_.heldBy() == currentThread);
 
 //Get disk_offset and new ppn
  size_t disk_offset = archmemory.getDiskLocation(vpn);  
  size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %ld to ppn %ld.\n", disk_offset, ppn);
  ustl::vector<IPTEntry*> virtual_page_infos = ipt_->moveEntry(IPTMapType::DISK_MAP, disk_offset, ppn);
  
  // update all the archmem to point to RAM
  lock_archmemories_in_right_order(virtual_page_infos);
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem;
    size_t vpn = virtual_page_info->vpn;
    debug(SWAPPING, "SwappingManager::swapInPage: vpn: %ld, archmemory: %p (disk offset %ld -> ppn %ld).\n", vpn, archmemory, disk_offset, ppn);
    archmemory->updatePageTableEntryForSwapIn(vpn, ppn);
  }

  // update refcount
  PageManager::instance()->ref_count_lock_.acquire();  //??
  PageManager::instance()->setReferenceCount(ppn, virtual_page_infos.size());
  PageManager::instance()->ref_count_lock_.release();

  // copying data from disk to ram
  disk_lock_.acquire();
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(ppn);
  bd_device_->readData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  // kprintf("Pagecontent after: <%s>\n", page_content);
  disk_lock_.release();
  total_disk_reads_++;
  unlock_archmemories(virtual_page_infos);

  debug(SWAPPING, "SwappingManager::swapInPage: Swap in vpn %ld finished", vpn);
  return 0;
}


void SwappingManager::lock_archmemories_in_right_order(ustl::vector<IPTEntry*> virtual_page_infos)
{
  debug(SWAPPING, "SwappingManager::unlock_archmemories: locking archmem in order of lowest ArchMemory* address to highest\n");
  ustl::vector<ArchMemory*> archmemories;
  for (IPTEntry* virtual_page_info : virtual_page_infos)
  {
    assert(virtual_page_info && "unlock_archmemories(): IPTEntry* is null");
    assert(virtual_page_info->archmem && "unlock_archmemories(): ArchMemory* is null");
    archmemories.push_back(virtual_page_info->archmem);
  }
  ustl::sort(archmemories.begin(), archmemories.end(), ustl::less<ArchMemory*>());

  for(ArchMemory* archmemory : archmemories)
  {
    archmemory->archmemory_lock_.acquire();
  }
}


void SwappingManager::unlock_archmemories(ustl::vector<IPTEntry*> virtual_page_infos)
{
  debug(SWAPPING, "SwappingManager::unlock_archmemories: unlocking archmem in order of highest ArchMemory* to lowest ArchMemory*\n");
  ustl::vector<ArchMemory*> archmemories;
  for (IPTEntry* virtual_page_info : virtual_page_infos)
  {
    assert(virtual_page_info && "unlock_archmemories(): IPTEntry* is null");
    assert(virtual_page_info->archmem && "unlock_archmemories(): ArchMemory* is null");
    archmemories.push_back(virtual_page_info->archmem);
  }
  ustl::sort(archmemories.begin(), archmemories.end(), ustl::greater<ArchMemory*>());

  for(ArchMemory* archmemory : archmemories)
  {
    archmemory->archmemory_lock_.release();
  }
}


int SwappingManager::getDiskWrites()
{
  return total_disk_writes_;
}



int SwappingManager::getDiskReads()
{
  return total_disk_reads_;
}





