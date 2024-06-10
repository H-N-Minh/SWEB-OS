#include "Loader.h" //
#include "UserThread.h"
#include "PageManager.h"
#include "SwappingManager.h"
#include "BDVirtualDevice.h"
#include "BDManager.h"
#include "IPTEntry.h"

size_t SwappingManager::disk_offset_counter_ = 0;
SwappingManager* SwappingManager::instance_ = nullptr;

SwappingManager::SwappingManager()
    : disk_lock_("disk_lock_"), pre_swap_lock_("pre_swap_lock_")
{
  assert(!instance_);
  instance_ = this;
  ipt_ = new IPTManager();
  bd_device_ = BDManager::getInstance()->getDeviceByNumber(3);
  bd_device_->setBlockSize(PAGE_SIZE);
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
  // lock in order IPT -> disk -> archmem
  assert(ipt_->IPT_lock_.heldBy() == currentThread);
  disk_lock_.acquire();
  ustl::vector<ArchmemIPT*> virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
  lock_archmemories_in_right_order(virtual_page_infos);

  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld to disk offset %ld.\n", ppn, disk_offset_counter_);
  ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset_counter_);

  ArchMemory* archmemory = virtual_page_infos[0]->archmem_;
  size_t vpn = virtual_page_infos[0]->vpn_;

  ArchMemoryMapping m = ArchMemory::resolveMapping(archmemory->page_map_level_4_, vpn);
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(m.pt[m.pti].page_ppn);

  bool write_status = bd_device_->writeData(disk_offset_counter_ * bd_device_->getBlockSize(), PAGE_SIZE, const_cast<char*>(page_content));

  if (!write_status) {
    kprintf("SwappingManager::swapOutPage: Failed to write data on disk.\n");
    return;
  }
  total_disk_writes_++;

  for (ArchmemIPT* virtual_page_info : virtual_page_infos) {
    archmemory = virtual_page_info->archmem_;
    vpn = virtual_page_info->vpn_;
    archmemory->updatePageTableEntryForSwapOut(vpn, disk_offset_counter_);
  }
  disk_offset_counter_++;

  PageManager::instance()->ref_count_lock_.acquire();
  PageManager::instance()->setReferenceCount(ppn, 0);
  PageManager::instance()->ref_count_lock_.release();

  unlock_archmemories(virtual_page_infos);
  disk_lock_.release();
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %ld finished", ppn);
}


//Only works if the page I want to swap in is in the archmemory of current thread
int SwappingManager::swapInPage(size_t disk_offset, ustl::vector<uint32>& preallocated_pages)
{
  assert(ipt_->IPT_lock_.isHeldBy((Thread*) currentThread));

  // get a free page to swap into
  size_t ppn = PageManager::getPreAllocatedPage(preallocated_pages);

  // lock the disk and all archmemories
  ustl::vector<ArchmemIPT*> virtual_page_infos = ipt_->disk_map_[disk_offset]->getArchmemIPTs();
  disk_lock_.acquire();
  lock_archmemories_in_right_order(virtual_page_infos);   // lock archmem before moveEntry, because moveEntry checks for this

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %ld to ppn %ld.\n", disk_offset, ppn);
  ipt_->moveEntry(IPTMapType::DISK_MAP, disk_offset, ppn);

  // update all the archmem to point to RAM
  for (ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::swapInPage: vpn: %ld, archmemory: %p (disk offset %ld -> ppn %ld).\n", vpn, archmemory, disk_offset, ppn);
    archmemory->updatePageTableEntryForSwapIn(vpn, ppn);
  }

  // update refcount
  PageManager::instance()->ref_count_lock_.acquire();  //??
  PageManager::instance()->setReferenceCount(ppn, virtual_page_infos.size());
  PageManager::instance()->ref_count_lock_.release();

  // copying data from disk to ram
  char* page_content = (char*)ArchMemory::getIdentAddressOfPPN(ppn);
  bool read_status = bd_device_->readData(disk_offset * bd_device_->getBlockSize(), PAGE_SIZE, page_content);

  if (!read_status) {
    kprintf("SwappingManager::swapOutPage: Failed to read data from disk.\n");
    return -1;
  }

  debug(SWAPPING, "SwappingManager::swapInPage: Successfully swapped in page from disk offset %zu to ppn %zu.\n", disk_offset, ppn);

  total_disk_reads_++;
  unlock_archmemories(virtual_page_infos);
  disk_lock_.release();

  debug(SWAPPING, "SwappingManager::swapInPage: Swap in: diskoffset %zu to ppn %zu finished", disk_offset, ppn);
  return 0;
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


int SwappingManager::getDiskWrites() const
{
  return total_disk_writes_;
}



int SwappingManager::getDiskReads() const
{
  return total_disk_reads_;
}

void SwappingManager::handlePreSwap()
{
  if (IPTManager::ENABLE_PRE_SWAP) {
    if (!IPTManager::instance()->checkMemoryThreshold()) return;

    IPTManager* ipt = IPTManager::instance();
    ustl::vector<size_t> pagesToPreSwap = ipt->getPagesToPreSwap();
    for (size_t ppn : pagesToPreSwap)
    {
      ipt->preSwapPage(ppn);
    }
  }
}

size_t SwappingManager::preSwapPageToDisk(const char* page_content)
{
  static size_t disk_offset_counter = 0;
  bool write_status = bd_device_->writeData(disk_offset_counter * bd_device_->getBlockSize(), PAGE_SIZE, const_cast<char*>(page_content));

  if (!write_status) {
    debug(PRESWAPPING,"SwappingManager::preSwapPageToDisk: Failed to write data to disk.\n");
    return -1;
  }

  debug(PRESWAPPING, "SwappingManager::preSwapPageToDisk: Successfully wrote data to disk offset %zu.\n", disk_offset_counter);

  return disk_offset_counter++;
}
