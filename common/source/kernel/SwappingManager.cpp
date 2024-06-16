#include "Loader.h"
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"
#include "SwappingThread.h"
#include "IPTEntry.h"

size_t SwappingManager::disk_offset_counter_ = 1;
SwappingManager* SwappingManager::instance_ = nullptr;

//ENABLE DISABLE PRE-SWAPPING
bool SwappingManager::pre_swap_enabled = true;  // true = enabled, false = disabled



SwappingManager::SwappingManager() : disk_lock_("disk_lock_"),
swapping_thread_finished_lock_("swapping_thread_finished_lock_"),
swapping_thread_finished_(&swapping_thread_finished_lock_, "swapping_thread_finished_")
{
  assert(!instance_);
  instance_ = this;
  ipt_ = new IPTManager();
  bd_device_ = BDManager::getInstance()->getDeviceByNumber(3);
  bd_device_->setBlockSize(PAGE_SIZE);
  debug(SWAPPING, "Block size %d.\n", bd_device_->getBlockSize());
}

SwappingManager* SwappingManager::instance()
{
  return instance_;
}

SwappingManager::~SwappingManager()
{

  SwappingThread::should_be_killed_ = true;
  swapping_thread_finished_lock_.acquire();
  while(SwappingThread::user_initialized_flag_)
  {
    swapping_thread_finished_.wait();
  }
  swapping_thread_finished_lock_.release();

  delete ipt_;
}

void SwappingManager::copyPageToDisk(size_t ppn)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  ustl::vector<ArchmemIPT*>& virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
  assert(virtual_page_infos.size() > 0);
  lockArchmemories(virtual_page_infos);

  if(isPageDirty(virtual_page_infos) || hasPageBeenDirty(virtual_page_infos))
  {
    // reset dirty flags
    resetDirtyBitSetBeenDirtyBits(virtual_page_infos);

    // find new disk offset
    size_t disk_offset = disk_offset_counter_;
    disk_offset_counter_++;

    // write to disk
    disk_lock_.acquire();
    writeToDisk(virtual_page_infos, disk_offset);
    disk_lock_.release();

    // copy instead of moving the page info
    ipt_->copyEntry(IPTMapType::RAM_MAP, ppn, disk_offset);

    // mark as pre-swapped
    ipt_->disk_map_[disk_offset]->isPreSwapped = true;
  }

  unlockArchmemories(virtual_page_infos);
}



//does only work if the page to swap out is also in the archmemory of the current thread
void SwappingManager::swapOutPage(size_t ppn)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  ustl::vector<ArchmemIPT*>& virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
  assert(virtual_page_infos.size() > 0);
  lockArchmemories(virtual_page_infos);  //TODOs not in right order yet

  setPagesToNotPresent(virtual_page_infos);

  if(pre_swap_enabled && ipt_->disk_map_.count(ppn) && ipt_->disk_map_[ppn]->isPreSwapped) {
    size_t disk_offset = ipt_->disk_map_[ppn]->last_disk_offset_;
    ipt_->finalizePreSwappedEntry(IPTMapType::RAM_MAP, ppn, disk_offset);

    // update the mapping
    updatePageTableEntriesForSwapOut(virtual_page_infos, disk_offset, ppn);

    // mark the page as not pre-swapped anymore
    ipt_->disk_map_[disk_offset]->isPreSwapped = false;

    PageManager::instance()->setReferenceCount(ppn, 0);
  }
  else {
    if (!isPageDirty(virtual_page_infos)) {
      if (!hasPageBeenDirty(virtual_page_infos)) {
        //     ipt_->removeEntry(IPTMapType::RAM_MAP, ppn);             //TODOs: entries should get deleted but should still be possible to unlock
        //     PageManager::instance()->setReferenceCount(ppn, 0);
        //     // printDebugInfos(virtual_page_infos, ppn, 0);
        //     updatePageTableEntriesForWriteBackToDisk(virtual_page_infos, ppn);
        //     unlockArchmemories(virtual_page_infos);

        //     discard_unchanged_page_++;
        //     return;
      } else {
        size_t disk_offset = ipt_->ram_map_[ppn]->last_disk_offset_;
        if (disk_offset == 0) {
          assert(0);//break is probably better
        }

        ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset);
        // printDebugInfos(virtual_page_infos, ppn, disk_offset);
        updatePageTableEntriesForSwapOut(virtual_page_infos, disk_offset, ppn);
        PageManager::instance()->setReferenceCount(ppn, 0);
        unlockArchmemories(virtual_page_infos);

        reuse_same_disk_location_++;
        return;
      }
    }
    resetDirtyBitSetBeenDirtyBits(virtual_page_infos);//todos: remove to check if is makes problems

    //Find free disk_offset
    size_t disk_offset = disk_offset_counter_;
    disk_offset_counter_++;

    //Move Page infos from ipt_map_ram to ipt_map_disk
    debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %p to disk offset %p.\n", (void *) ppn, (void *) disk_offset);
    ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset);

    // printDebugInfos(virtual_page_infos, ppn, disk_offset);

    //write to disk
    disk_lock_.acquire();
    writeToDisk(virtual_page_infos, disk_offset);
    disk_lock_.release();
    updatePageTableEntriesForSwapOut(virtual_page_infos, disk_offset, ppn);

    PageManager::instance()->setReferenceCount(ppn, 0);
  }

  unlockArchmemories(virtual_page_infos);
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %p finished", (void*)ppn);
}


//Only works if the page I want to swap in is in the archmemory of current thread
int SwappingManager::swapInPage(size_t disk_offset, ustl::vector<uint32>& preallocated_pages)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  if(!ipt_->isKeyInMap(disk_offset, DISK_MAP))
  {
    //At this offset is nothing in the IPT-Map somebody else was faster
    return -1;
  }

  ustl::vector<ArchmemIPT*>& virtual_page_infos = ipt_->disk_map_[disk_offset]->getArchmemIPTs();
  assert(virtual_page_infos.size() > 0);
  lockArchmemories(virtual_page_infos);

  //Get new ppn
  size_t ppn = PageManager::instance()->getPreAllocatedPage(preallocated_pages);

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %p to ppn %p.\n", (void*)disk_offset, (void*)ppn);

  // If the disk_offset is present in RAM map as preswapped page
  if (ipt_->isKeyInMap(ppn, IPTMapType::RAM_MAP) && ipt_->ram_map_[ppn]->isPreSwapped) {
    // The preswapped entry is now active, finalize it
    ipt_->finalizePreSwappedEntry(IPTMapType::DISK_MAP, disk_offset, ppn);
    // Mark the page as not pre-swapped anymore
    ipt_->ram_map_[ppn]->isPreSwapped = false;
  }
  else {
    // Non-preswapped page, move from disk to RAM
    ipt_->moveEntry(IPTMapType::DISK_MAP, disk_offset, ppn);

    // Read from disk
    disk_lock_.acquire();
    readFromDisk(disk_offset, ppn);
    disk_lock_.release();
  }

  // Update Page Table Entries
  updatePageTableEntriesForSwapIn(virtual_page_infos, ppn, disk_offset);

  // Set Reference Count
  PageManager::instance()->setReferenceCount(ppn, virtual_page_infos.size());

  ipt_->ram_map_[ppn]->last_disk_offset_ = disk_offset;

  unlockArchmemories(virtual_page_infos);

  debug(SWAPPING, "SwappingManager::swapInPage: Swap in from disk_offset %p finished", (void*)disk_offset);
  return 0;
}


void SwappingManager::lockArchmemories(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    archmemory->archmemory_lock_.acquire();
  }
}

void SwappingManager::unlockArchmemories(ustl::vector<ArchmemIPT*>& virtual_page_infos)
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
    debug(SWAPPING, "SwappingManager::swapOutPage: vpn: %p, archmemory: %p (ppn %p -> disk offset %p).\n", (void*)vpn, archmemory, (void*)ppn, (void*)disk_offset);
  }
}

void SwappingManager::writeToDisk(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t disk_offset)
{
  ArchMemory* archmemory = virtual_page_infos[0]->archmem_;
  size_t vpn = virtual_page_infos[0]->vpn_;
  ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);

  int status = bd_device_->writeData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);

  assert(status >= 0 && "Error: bd_device_->writeData failed");

  total_disk_writes_++;
}

void SwappingManager::readFromDisk(size_t disk_offset, size_t ppn)
{
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(ppn);

  int bytesRead = bd_device_->readData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);

  assert(bytesRead >= 0 && "Error: bd_device_->readData failed");

  total_disk_reads_++;
}

void SwappingManager::updatePageTableEntriesForSwapOut(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t disk_offset, size_t ppn)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::swapOutPage: vpn: %p, archmemory: %p (ppn %p -> disk offset %p).\n", (void*)vpn, archmemory, (void*)ppn, (void*)disk_offset);
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
    debug(SWAPPING, "SwappingManager::swapInPage: vpn: %p, archmemory: %p (disk offset %p -> ppn %p).\n", (void*)vpn, archmemory, (void*)disk_offset, (void*)ppn);
    archmemory->updatePageTableEntryForSwapIn(vpn, ppn);
  }
}


void SwappingManager::lock_archmemories_in_right_order(ustl::vector<ArchmemIPT*> &virtual_page_infos)
{
  debug(SWAPPING, "SwappingManager::unlock_archmemories: locking archmem in order of lowest ArchMemory* address to highest\n");
  ustl::vector<ArchMemory*> archmemories;
  for (ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    assert(virtual_page_info && "unlock_archmemories(): ArchmemIPT* is null");
    assert(virtual_page_info->archmem_ && "unlock_archmemories(): ArchMemory* is null");
    archmemories.push_back(virtual_page_info->archmem_);
  }
  ustl::sort(archmemories.begin(), archmemories.end(), ustl::less<ArchMemory*>());

  for(ArchMemory* archmemory : archmemories)
  {
    archmemory->archmemory_lock_.acquire();
  }
}


void SwappingManager::unlock_archmemories(ustl::vector<ArchmemIPT*> &virtual_page_infos)
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


int SwappingManager::getDiskWrites()
{
  return total_disk_writes_;
}


int SwappingManager::getDiskReads()
{
  return total_disk_reads_;
}


bool SwappingManager::isPageDirty(ustl::vector<ArchmemIPT*> &virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    
    if(archmemory->isBitSet(vpn, BitType::DIRTY, true))
    {
      return true;
    }
  }
  return false;
}



void SwappingManager::updatePageTableEntriesForWriteBackToDisk(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t ppn)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::writeBackPage: vpn: %p, archmemory: %p (ppn %p).\n", (void*)vpn, archmemory, (void*)ppn);
    archmemory->updatePageTableEntryForWriteBackToDisk(vpn);
  }
}

void SwappingManager::setPagesToNotPresent(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    archmemory->setPageTableEntryToNotPresent(vpn);
  }
}

bool SwappingManager::hasPageBeenDirty(ustl::vector<ArchmemIPT*> &virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;

    if(archmemory->isBitSet(vpn, BitType::BEEN_DIRTY, true))
    {
      return true;
    }
  }
  return false;
}


void SwappingManager::resetDirtyBitSetBeenDirtyBits(ustl::vector<ArchmemIPT*>& virtual_page_infos)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    archmemory->resetDirtyBitSetBeenDirtyBits(vpn);
  }
}



