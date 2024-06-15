#include "Loader.h"
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"
#include "SwappingThread.h"
#include "IPTEntry.h"

size_t SwappingManager::disk_offset_counter_ = 0;
SwappingManager* SwappingManager::instance_ = nullptr;

//ENABLE DISABLE PRE-SWAPPING
bool SwappingManager::pre_swap_enabled = false;  // true = enabled, false = disabled



SwappingManager::SwappingManager() : disk_lock_("disk_lock_"),
swapping_thread_finished_lock_("swapping_thread_finished_lock_"),
swapping_thread_finished_(&swapping_thread_finished_lock_, "swapping_thread_finished_")
{
  assert(!instance_);
  instance_ = this;
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
  if(ASYNCHRONOUS_SWAPPING)
  {
    SwappingThread::should_be_killed_ = true;
    swapping_thread_finished_lock_.acquire();
    while(SwappingThread::user_initialized_flag_)
    {
      swapping_thread_finished_.wait();
    }
    swapping_thread_finished_lock_.release();
  }

  delete ipt_;
}

//bool SwappingManager::preSwapPage(size_t ppn)
//{
//  if (!pre_swap_enabled) return false;  // Return immediately if preswapping is disabled
//
//  assert(ipt_->IPT_lock_.heldBy() == currentThread);
//  disk_lock_.acquire();
//  ustl::vector<ArchmemIPT*> virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
//  lock_archmemories_in_right_order(virtual_page_infos);
//
//  debug(PRESWAPPING, "SwappingManager::preSwapPage: Pre-swap page with ppn %ld to disk offset %ld.\n", ppn, disk_offset_counter_);
//
//  ArchMemory* archmemory = virtual_page_infos[0]->archmem_;
//  size_t vpn = virtual_page_infos[0]->vpn_;
//
//  ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
//  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
//
//  bool write_status = bd_device_->writeData(disk_offset_counter_ * bd_device_->getBlockSize(), PAGE_SIZE, const_cast<char*>(page_content));
//
//  if (!write_status) {
//    kprintf("SwappingManager::preSwapPage: Failed to write data on disk.\n");
//    unlock_archmemories(virtual_page_infos);
//    disk_lock_.release();
//    return false;
//  }
//
//  for (ArchmemIPT* virtual_page_info : virtual_page_infos) {
//    archmemory = virtual_page_info->archmem_;
//    vpn = virtual_page_info->vpn_;
//    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset_counter_);
//  }
//
//  // If the entry disk map doesn't already contain the current entry, add it
//  if (!ipt_->isEntryInMap(disk_offset_counter_, IPTMapType::DISK_MAP, archmemory)) {
//    ipt_->copyEntryToPreswap(IPTMapType::RAM_MAP, ppn);
//  } else {
//    debug(PRESWAPPING,"SwappingManager::preSwapPage: Entry already exists in the disk map, skipping move.\n");
//  }
//
//  disk_offset_counter_++;
//  pre_swapped_pages.push_back(ppn);
//
//  PageManager::instance()->ref_count_lock_.acquire();
//  PageManager::instance()->setReferenceCount(ppn, 0);
//  PageManager::instance()->ref_count_lock_.release();
//
//  unlock_archmemories(virtual_page_infos);
//  disk_lock_.release();
//  debug(PRESWAPPING, "SwappingManager::preSwapPage: Pre-swap page with ppn %ld finished", ppn);
//  return true;
//}

bool SwappingManager::preSwapPage(size_t ppn)
{
  if (!pre_swap_enabled) return false;  // Return immediately if preswapping is disabled

  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  ustl::vector<ArchmemIPT*> virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
  lockArchmemorys(virtual_page_infos);  //TODOs not in right order yet

  disk_lock_.acquire();

  debug(PRESWAPPING, "SwappingManager::preSwapPage: Pre-swap page with ppn %ld to disk offset %ld.\n", ppn, disk_offset_counter_);

  ArchMemory* archmemory = virtual_page_infos[0]->archmem_;
  size_t vpn = virtual_page_infos[0]->vpn_;

  ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);

  bool write_status = bd_device_->writeData(disk_offset_counter_ * bd_device_->getBlockSize(), PAGE_SIZE, const_cast<char*>(page_content));

  if (!write_status) {
    kprintf("SwappingManager::preSwapPage: Failed to write data on disk.\n");
    unlockArchmemorys(virtual_page_infos);
    disk_lock_.release();
    return false;
  }

  disk_offset_counter_++;
  pre_swapped_pages.push_back(ppn);

  for (ArchmemIPT* virtual_page_info : virtual_page_infos) {
    archmemory = virtual_page_info->archmem_;
    vpn = virtual_page_info->vpn_;
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset_counter_);
  }

  if (!ipt_->isEntryInMap(disk_offset_counter_, IPTMapType::DISK_MAP, archmemory, vpn)) {
    ipt_->copyEntryToPreswap(IPTMapType::RAM_MAP, ppn);
  } else {
    debug(PRESWAPPING,"SwappingManager::preSwapPage: Entry already exists in the disk map, skipping move.\n");
  }

  PageManager::instance()->setReferenceCount(ppn, 0);

  unlockArchmemorys(virtual_page_infos);
  disk_lock_.release();
  debug(PRESWAPPING, "SwappingManager::preSwapPage: Pre-swap page with ppn %ld finished", ppn);
  return true;
}

//does only work if the page to swap out is also in the archmemory of the current thread
void SwappingManager::swapOutPage(size_t ppn)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  ustl::vector<ArchmemIPT*>& virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
  lockArchmemorys(virtual_page_infos);  //TODOs not in right order yet

  if(isPageUnchanged(virtual_page_infos))
  {
    ipt_->removeEntry(IPTMapType::RAM_MAP, ppn);             //TODOs: entries should get deleted but should still be possible to unlock
    PageManager::instance()->setReferenceCount(ppn, 0);
    updatePageTableEntriesForWriteBackToDisk(virtual_page_infos);
    unlockArchmemorys(virtual_page_infos);
    return;
  }

  //Find free disk_offset
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

//void SwappingManager::swapOutPage(size_t ppn)
//{
//  assert(ipt_->IPT_lock_.heldBy() == currentThread);
//  disk_lock_.acquire();
//  auto it = ustl::find(pre_swapped_pages.begin(), pre_swapped_pages.end(), ppn);
//  bool preswapped = it != pre_swapped_pages.end();
//  ustl::vector<ArchmemIPT*> virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
//  lock_archmemories_in_right_order(virtual_page_infos);
//
//  if (preswapped)
//  {
//    // Use the new method instead of moveEntry()
//    ipt_->movePreswapedToDisk(IPTMapType::RAM_MAP, ppn);
//    pre_swapped_pages.erase(it);
//  }
//  else{
//    debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld to disk offset %ld.\n", ppn, disk_offset_counter_);
//    ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset_counter_);
//
//    ArchMemory* archmemory = virtual_page_infos[0]->archmem_;
//    size_t vpn = virtual_page_infos[0]->vpn_;
//
//    ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
//    char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
//
//    bool write_status = bd_device_->writeData(disk_offset_counter_ * bd_device_->getBlockSize(), PAGE_SIZE, const_cast<char*>(page_content));
//
//    if (!write_status) {
//      kprintf("SwappingManager::swapOutPage: Failed to write data on disk.\n");
//      unlock_archmemories(virtual_page_infos);
//      disk_lock_.release();
//      return;
//    }
//    total_disk_writes_++;
//
//    for (ArchmemIPT* virtual_page_info : virtual_page_infos) {
//      archmemory = virtual_page_info->archmem_;
//      vpn = virtual_page_info->vpn_;
//      archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset_counter_);
//    }
//    disk_offset_counter_++;
//  }
//
//  PageManager::instance()->ref_count_lock_.acquire();
//  PageManager::instance()->setReferenceCount(ppn, 0);
//  PageManager::instance()->ref_count_lock_.release();
//
//  unlock_archmemories(virtual_page_infos);
//  disk_lock_.release();
//  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld finished", ppn);
//}

//Only works if the page i want to swap in is in the archmemory of current thread
int SwappingManager::swapInPage(size_t disk_offset, ustl::vector<uint32>& preallocated_pages)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  if(!ipt_->isKeyInMap(disk_offset, DISK_MAP))
  {
    //At this offset is nothing in the IPT-Map somebody else was faster
    return -1;
  }

  ustl::vector<ArchmemIPT*>& virtual_page_infos = ipt_->disk_map_[disk_offset]->getArchmemIPTs();
  lockArchmemorys(virtual_page_infos);

 //Get new ppn
  size_t ppn = PageManager::instance()->getPreAllocatedPage(preallocated_pages);

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %ld to ppn %ld.\n", disk_offset, ppn);
  ipt_->moveEntry(IPTMapType::DISK_MAP, disk_offset, ppn);

  updatePageTableEntriesForSwapIn(virtual_page_infos, ppn, disk_offset);

  PageManager::instance()->setReferenceCount(ppn, virtual_page_infos.size());

  disk_lock_.acquire();
  readFromDisk(disk_offset, ppn);
  disk_lock_.release();

  unlockArchmemorys(virtual_page_infos);

  debug(SWAPPING, "SwappingManager::swapInPage: Swap in from disk_offset %ld finished", disk_offset);
  return 0;
}

void SwappingManager::lockArchmemorys(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    archmemory->archmemory_lock_.acquire();
  }
}

void SwappingManager::unlockArchmemorys(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    archmemory->archmemory_lock_.release();
  }
}

void SwappingManager::printDebugInfos(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t ppn, size_t disk_offset)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::swapOutPage: vpn: %ld, archmemory: %p (ppn %ld -> disk offset %ld).\n", vpn, archmemory, ppn, disk_offset);
  }
}

void SwappingManager::writeToDisk(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t disk_offset)
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

void SwappingManager::updatePageTableEntriesForSwapOut(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t disk_offset)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset);
  }
}

void SwappingManager::updatePageTableEntriesForSwapIn(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t ppn, size_t disk_offset)
{
  // update all the archmem to point to RAM
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::swapInPage: vpn: %ld, archmemory: %p (disk offset %ld -> ppn %ld).\n", vpn, archmemory, disk_offset, ppn);
    archmemory->updatePageTableEntryForSwapIn(vpn, ppn);
  }
}

void SwappingManager::lock_archmemories_in_right_order(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  debug(SWAPPING, "SwappingManager::lock_archmemories_in_right_order: locking archmem in order of lowest ArchMemory* address to highest\n");
  ustl::vector<ArchMemory*> archmemories;
  for (ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    assert(virtual_page_info && "lock_archmemories_in_right_order(): ArchmemIPT* is null");
    assert(virtual_page_info->archmem_ && "lock_archmemories_in_right_order(): ArchMemory* is null");
    archmemories.push_back(virtual_page_info->archmem_);
  }
  ustl::sort(archmemories.begin(), archmemories.end(), ustl::less<ArchMemory*>());

  for(ArchMemory* archmemory : archmemories)
  {
    archmemory->archmemory_lock_.acquire();
  }
}

void SwappingManager::unlock_archmemories(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  debug(SWAPPING, "SwappingManager::unlock_archmemories: unlocking archmem in order of highest ArchMemory* to lowest ArchMemory*\n");
  ustl::vector<ArchMemory*> archmemories;
  for (ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    assert(virtual_page_info && "unlock_archmemories(): ArchmemIPT* is null");
    assert(virtual_page_info->archmem_ && "unlock_archmemories(): ArchMemory* is null");
    archmemories.push_back(virtual_page_info->archmem_);
  }
  ustl::sort(archmemories.begin(), archmemories.end(), ustl::greater<ArchMemory*>());

  for(ArchMemory* archmemory : archmemories)
  {
    archmemory->archmemory_lock_.release();
  }
}


int SwappingManager::getDiskWrites() const
{
  return total_disk_writes_;
}


int SwappingManager::getDiskReads() const
{
  return total_disk_reads_;
}


bool SwappingManager::isPageUnchanged(ustl::vector<ArchmemIPT*> &virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;

    if(archmemory->isPageDirty(vpn))
    {
      return false;
    }
  }
  return true;
}



void SwappingManager::updatePageTableEntriesForWriteBackToDisk(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    archmemory->updatePageTableEntryForWriteBackToDisk(vpn);
  }
}



