#include "Loader.h" //
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"

size_t SwappingManager::disk_offset_counter_ = 0;
SwappingManager* SwappingManager::instance_ = nullptr;

SwappingManager::SwappingManager() : disk_lock_("disk_lock_"), pre_swap_lock_("pre_swap_lock_")
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
void SwappingManager::swapOutPageAsynchronous(size_t ppn)
{
  // lock in order IPT -> disk -> archmem
  assert(ipt_->IPT_lock_.heldBy() == currentThread);
  disk_lock_.acquire();
  ustl::vector<IPTEntry*> virtual_page_infos = ipt_->getIptEntriesFromKey(ppn, IPTMapType::RAM_MAP);
  lock_archmemories_in_right_order(virtual_page_infos);   // lock archmem before moveEntry, because moveEntry checks for this

  //Move Page infos from ipt_map_ram to ipt_map_disk
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld to disk offset %ld.\n", ppn, disk_offset_counter_);
  ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset_counter_);

  // for debuging
  printDebugInfos(virtual_page_infos, ppn, disk_offset_counter_);
  
  writeToDisk(virtual_page_infos, disk_offset_counter_);

  // updating all archmem to point to disk
  updatePageTableEntriesForSwapOut(virtual_page_infos, disk_offset_counter_);
  disk_offset_counter_++;

  PageManager::instance()->setReferenceCount(ppn, 0);

  unlock_archmemories(virtual_page_infos);
  disk_lock_.release();
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld finished", ppn);
}

//Only works if the page i want to swap in is in the archmemory of current thread
int SwappingManager::swapInPageAsynchronous(size_t disk_offset, ustl::vector<uint32>& preallocated_pages)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);
  
  // get a free page to swap into
  size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);

  // lock the disk and all archmemories
  ustl::vector<IPTEntry*> virtual_page_infos = ipt_->getIptEntriesFromKey(disk_offset, IPTMapType::DISK_MAP);
  disk_lock_.acquire();
  lock_archmemories_in_right_order(virtual_page_infos);   // lock archmem before moveEntry, because moveEntry checks for this

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %ld to ppn %ld.\n", disk_offset, ppn);
  ipt_->moveEntry(IPTMapType::DISK_MAP, disk_offset, ppn);
  
  // update all the archmem to point to RAM
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::swapInPage: vpn: %ld, archmemory: %p (disk offset %ld -> ppn %ld).\n", vpn, archmemory, disk_offset, ppn);
    archmemory->updatePageTableEntryForSwapIn(vpn, ppn);
  }

  // update refcount
  PageManager::instance()->setReferenceCount(ppn, virtual_page_infos.size());

  // copying data from disk to ram
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(ppn);
  bd_device_->readData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  // kprintf("Pagecontent after: <%s>\n", page_content);
  total_disk_reads_++;
  unlock_archmemories(virtual_page_infos);
  disk_lock_.release();

  debug(SWAPPING, "SwappingManager::swapInPage: Swap in: diskoffset %zu to ppn %zu finished", disk_offset, ppn);
  return 0;
}


void SwappingManager::lock_archmemories_in_right_order(ustl::vector<IPTEntry*> &virtual_page_infos)
{
  debug(SWAPPING, "SwappingManager::unlock_archmemories: locking archmem in order of lowest ArchMemory* address to highest\n");
  ustl::vector<ArchMemory*> archmemories;
  for (IPTEntry* virtual_page_info : virtual_page_infos)
  {
    assert(virtual_page_info && "unlock_archmemories(): IPTEntry* is null");
    assert(virtual_page_info->archmem_ && "unlock_archmemories(): ArchMemory* is null");
    archmemories.push_back(virtual_page_info->archmem_);
  }
  ustl::sort(archmemories.begin(), archmemories.end(), ustl::less<ArchMemory*>());

  for(ArchMemory* archmemory : archmemories)
  {
    archmemory->archmemory_lock_.acquire();
  }
}


void SwappingManager::unlock_archmemories(ustl::vector<IPTEntry*> &virtual_page_infos)
{
  debug(SWAPPING, "SwappingManager::unlock_archmemories: unlocking archmem in order of highest ArchMemory* to lowest ArchMemory*\n");
  ustl::vector<ArchMemory*> archmemories;
  for (IPTEntry* virtual_page_info : virtual_page_infos)
  {
    assert(virtual_page_info && "unlock_archmemories(): IPTEntry* is null");
    assert(virtual_page_info->archmem_ && "unlock_archmemories(): ArchMemory* is null");
    archmemories.push_back(virtual_page_info->archmem_);
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



//does only work if the page to swap out is also in the archmemory of the current thread
void SwappingManager::swapOutPage(size_t ppn)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  ustl::vector<IPTEntry*> virtual_page_infos = ipt_->getIptEntriesFromKey(ppn, IPTMapType::RAM_MAP);
  lockArchmemorys(virtual_page_infos);  //TODOs not in right order yet

  //Find free disk_offset (TODOs)
  size_t disk_offset = disk_offset_counter_;
  disk_offset_counter_++;

  //Move Page infos from ipt_map_ram to ipt_map_disk
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld to disk offset %ld.\n", ppn, disk_offset);
  ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset);

  printDebugInfos(virtual_page_infos, ppn, disk_offset);

  //write to disk
  disk_lock_.acquire();
  writeToDisk(virtual_page_infos, disk_offset);
  disk_lock_.release();
  updatePageTableEntriesForSwapOut(virtual_page_infos, disk_offset);

  PageManager::instance()->setReferenceCount(ppn, 0);

  unlockArchmemorys(virtual_page_infos);
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld finished", ppn);
}

//Only works if the page i want to swap in is in the archmemory of current thread
int SwappingManager::swapInPage(size_t disk_offset, ustl::vector<uint32>& preallocated_pages)
{
  ArchMemory& archmemory = currentThread->loader_->arch_memory_; //TODOs Select the right archmemory not nessessary the one of the current thread
  
  assert(ipt_->IPT_lock_.heldBy() == currentThread);
  assert(archmemory.archmemory_lock_.heldBy() == currentThread); //Todo remove

  //TODOs check if data is still swappped out or in ipt lock then return

  ustl::vector<IPTEntry*> virtual_page_infos = ipt_->getIptEntriesFromKey(disk_offset, IPTMapType::DISK_MAP);
  lockArchmemoriesInRightOrder(virtual_page_infos);
 
 //Get new ppn
  size_t ppn = PageManager::instance()->getPreAlocatedPage(preallocated_pages);

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %ld to ppn %ld.\n", disk_offset, ppn);
  ipt_->moveEntry(IPTMapType::DISK_MAP, disk_offset, ppn);

  updatePageTableEntriesForSwapIn(virtual_page_infos, ppn, disk_offset);
  
  PageManager::instance()->setReferenceCount(ppn, virtual_page_infos.size());

  disk_lock_.acquire();
  readFromDisk(disk_offset, ppn);
  disk_lock_.release();

  unlockArchmemories(virtual_page_infos);

  debug(SWAPPING, "SwappingManager::swapInPage: Swap in from disk_offset %ld finished", disk_offset);
  return 0;
}



//TODOs: Does not lock them in the right order yet and does not lock current thread
void SwappingManager::lockArchmemoriesInRightOrder(ustl::vector<IPTEntry*>& virtual_page_infos)
{
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    if(archmemory != &currentThread->loader_->arch_memory_)
    {
      archmemory->archmemory_lock_.acquire();
    }
  }
}


//TODOs: Does at the moment not unlock the current thread
void SwappingManager::unlockArchmemories(ustl::vector<IPTEntry*>& virtual_page_infos)
{
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    if(archmemory != &currentThread->loader_->arch_memory_)
    {
      archmemory->archmemory_lock_.release();
    }
  }
}


void SwappingManager::lockArchmemorys(ustl::vector<IPTEntry*>& virtual_page_infos)
{
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    archmemory->archmemory_lock_.acquire();
  }
}

void SwappingManager::unlockArchmemorys(ustl::vector<IPTEntry*>& virtual_page_infos)
{
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    archmemory->archmemory_lock_.release();
  }
}

void SwappingManager::printDebugInfos(ustl::vector<IPTEntry*>& virtual_page_infos, size_t ppn, size_t disk_offset)
{
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::swapOutPage: vpn: %ld, archmemory: %p (ppn %ld -> disk offset %ld).\n", vpn, archmemory, ppn, disk_offset);
  }
}

void SwappingManager::writeToDisk(ustl::vector<IPTEntry*>& virtual_page_infos, size_t disk_offset)
{
  ArchMemory* archmemory = virtual_page_infos[0]->archmem_;
  size_t vpn = virtual_page_infos[0]->vpn_;
  ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
  bd_device_->writeData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  total_disk_writes_++;
}

void SwappingManager::readFromDisk(size_t disk_offset, size_t ppn)
{
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(ppn);
  bd_device_->readData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);
  total_disk_reads_++;
}

void SwappingManager::updatePageTableEntriesForSwapOut(ustl::vector<IPTEntry*>& virtual_page_infos, size_t disk_offset)
{
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset);
  }
}

void SwappingManager::updatePageTableEntriesForSwapIn(ustl::vector<IPTEntry*>& virtual_page_infos, size_t ppn, size_t disk_offset)
{
  for(IPTEntry* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::swapInPage: vpn: %ld, archmemory: %p (disk offset %ld -> ppn %ld).\n", vpn, archmemory, disk_offset, ppn);
    archmemory->updatePageTableEntryForSwapIn(vpn, ppn);
  }
}




