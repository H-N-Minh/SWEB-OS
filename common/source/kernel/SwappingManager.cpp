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
bool SwappingManager::pre_swap_enabled = false;

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

}



//does only work if the page to swap out is also in the archmemory of the current thread
void SwappingManager::swapOutPage(size_t ppn)
{
  assert(ipt_->IPT_lock_.heldBy() == currentThread);

  ustl::vector<ArchmemIPT*>& virtual_page_infos = ipt_->ram_map_[ppn]->getArchmemIPTs();
  assert(virtual_page_infos.size() > 0);
  lockArchmemorys(virtual_page_infos);

  setPagesToNotPresent(virtual_page_infos);

  if(!isPageDirty(virtual_page_infos))
  {
    if(!hasPageBeenDirty(virtual_page_infos))
    {
      ipt_->removeEntry(IPTMapType::RAM_MAP, ppn);  //remove it completly dont need to swap out bc if we do we cant just take it from the binary
      PageManager::instance()->setReferenceCount(ppn, 0);
      // printDebugInfos(virtual_page_infos, ppn, 0);
      updatePageTableEntriesForDiscardPage(virtual_page_infos, ppn);
      unlockArchmemorys(virtual_page_infos);
      for(auto& el : virtual_page_infos)
      {
        delete el;
      }
      virtual_page_infos.clear();


      discard_unchanged_page_++;
      return;
    }
    else
    {
      size_t disk_offset = ipt_->ram_map_[ppn]->last_disk_offset_;
      if(!(disk_offset == 0))
      {
        ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset);
        // printDebugInfos(virtual_page_infos, ppn, disk_offset);
        updatePageTableEntriesForSwapOut(virtual_page_infos, disk_offset, ppn);
        PageManager::instance()->setReferenceCount(ppn, 0);
        unlockArchmemorys(virtual_page_infos);

        reuse_same_disk_location_++;
        return;
      }
    }
  }
  resetDirtyBitSetBeenDirtyBits(virtual_page_infos);

  //Find free disk_offset
  size_t disk_offset = disk_offset_counter_;
  disk_offset_counter_++;

  //Move Page infos from ipt_map_ram to ipt_map_disk
  debug(SWAPPING, "SwappingManager::swapOutPage: Swap out page with ppn %p to disk offset %p.\n", (void*)ppn, (void*)disk_offset);
  ipt_->moveEntry(IPTMapType::RAM_MAP, ppn, disk_offset);

  // printDebugInfos(virtual_page_infos, ppn, disk_offset);

  //write to disk
  disk_lock_.acquire();
  writeToDisk(virtual_page_infos, disk_offset);
  disk_lock_.release();
  updatePageTableEntriesForSwapOut(virtual_page_infos, disk_offset, ppn);

  PageManager::instance()->setReferenceCount(ppn, 0);

  unlockArchmemorys(virtual_page_infos);
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
  lockArchmemorys(virtual_page_infos);

 //Get new ppn
  size_t ppn = PageManager::instance()->getPreAllocatedPage(preallocated_pages);

  //Move Page infos from  ipt_map_disk to ipt_map_ram
  debug(SWAPPING, "SwappingManager::swapInPage: Swap in page with disk_offset %p to ppn %p.\n", (void*)disk_offset, (void*)ppn);
  ipt_->moveEntry(IPTMapType::DISK_MAP, disk_offset, ppn);

  updatePageTableEntriesForSwapIn(virtual_page_infos, ppn, disk_offset);

  PageManager::instance()->setReferenceCount(ppn, virtual_page_infos.size());

  disk_lock_.acquire();
  readFromDisk(disk_offset, ppn);
  disk_lock_.release();

  ipt_->ram_map_[ppn]->last_disk_offset_ = disk_offset;

  unlockArchmemorys(virtual_page_infos);

  debug(SWAPPING, "SwappingManager::swapInPage: Swap in from disk_offset %p finished", (void*)disk_offset);
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
    debug(SWAPPING, "SwappingManager::swapOutPage: vpn: %p, archmemory: %p (ppn %p -> disk offset %p).\n", (void*)vpn, archmemory, (void*)ppn, (void*)disk_offset);
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



void SwappingManager::updatePageTableEntriesForDiscardPage(ustl::vector<ArchmemIPT*>& virtual_page_infos, size_t ppn)
{
  for(ArchmemIPT* virtual_page_info : virtual_page_infos)
  {
    ArchMemory* archmemory = virtual_page_info->archmem_;
    size_t vpn = virtual_page_info->vpn_;
    debug(SWAPPING, "SwappingManager::writeBackPage: vpn: %p, archmemory: %p (ppn %p).\n", (void*)vpn, archmemory, (void*)ppn);
    archmemory->updatePageTableEntryForDiscardPage(vpn);
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

void SwappingManager::swapSearch(const char* searchString)
{
  size_t searchStringLen = strlen(searchString);
  size_t blockSize = bd_device_->getBlockSize(); //size of a single block on the disk, retrieved from the block device
  size_t numBlocks = disk_offset_counter_; //total number of blocks used on the disk
  char* buffer = new char[blockSize * 2]; //buffer to handle overlap between blocks

  //loop through disk blocks
  for (size_t blockNum = 2; blockNum < numBlocks; ++blockNum)
  {
    //reads the current block into the first half of the buffer
    bd_device_->readData(blockNum * blockSize, blockSize, buffer);
    //reads the next block into the second half of the buffer
    if (blockNum + 1 < numBlocks)
    {
      bd_device_->readData((blockNum + 1) * blockSize, blockSize, buffer + blockSize);
    }
    else
    {
      //zero out the second part if no next block
      memset(buffer + blockSize, 0, blockSize);
    }

    //search for the string in the buffer
    for (size_t i = 0; i < blockSize; ++i)
    {
      if (strncmp(buffer + i, searchString, searchStringLen) == 0)
      {
        // Found the string, print until null character
        const char* foundString = buffer + i;
        while (*foundString != '\0')
        {
          debug(SWAPPING, "swapSearch found %s\n", *foundString);
          ++foundString;
        }
        delete[] buffer;
        return;
      }
    }
  }
  delete[] buffer;
  debug(SWAPPING, "swapSearch no string found %s\n");
}

int SwappingManager::compareBlocks(size_t block_num_1, size_t block_num_2)
{
    // Lock the disk to ensure exclusive access
    disk_lock_.acquire();

    // Allocate temporary buffers for reading the block contents
    char* block1 = new char[PAGE_SIZE];
    char* block2 = new char[PAGE_SIZE];

    // Read the block contents
    bd_device_->readData(block_num_1 * PAGE_SIZE, PAGE_SIZE, block1);
    bd_device_->readData(block_num_2 * PAGE_SIZE, PAGE_SIZE, block2);

    // Unlock the disk after reading
    disk_lock_.release();

    // Compare the two blocks
    bool are_equal = memcmp(block1, block2, PAGE_SIZE) == 0;

    // Clean up the temporary buffers
    delete[] block1;
    delete[] block2;

    // Return the comparison result
    return are_equal ? 0 : -1;
}