#include "PageManager.h"
#include "new.h"
#include "offsets.h"
#include "paging-definitions.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "KernelMemoryManager.h"
#include "assert.h"
#include "Bitmap.h"
#include "ArchThreads.h"

PageManager pm;

PageManager* PageManager::instance_ = nullptr;

/**
 * @brief Retrieves the instance of the PageManager class.
 *
 * If an instance does not exist, it creates a new instance using placement new and returns it.
 *
 * @return A pointer to the instance of the PageManager class.
 */
PageManager* PageManager::instance()
{
  if (unlikely(!instance_))
    new (&pm) PageManager();
  return instance_;
}

/**
 * @class PageManager
 * @brief The PageManager class represents a manager for pages.
 *
 * The PageManager class manages the creation, deletion, and locking of pages.
 * It provides thread-safe access to the pages using a lock.
 *
 * The lock is created in the constructor and is passed a name to uniquely identify it.
 * This allows for easier debugging and tracking of locks.
 */
PageManager::PageManager() 
    : page_reference_counts_lock_("PageManager::page_reference_counts_lock_"),
      inverted_page_table_lock_("PageManager::inverted_page_table_lock_"), page_manager_lock_("PageManager::page_manager_lock_")
{
  assert(!instance_);
  instance_ = this;
  assert(!KernelMemoryManager::instance_);
  number_of_pages_ = 0;
  lowest_unreserved_page_ = 0;

  size_t num_mmaps = ArchCommon::getNumUseableMemoryRegions();

  size_t highest_address = 0, used_pages = 0;

  //Determine Amount of RAM
  for (size_t i = 0; i < num_mmaps; ++i)
  {
    pointer start_address = 0, end_address = 0, type = 0;
    ArchCommon::getUsableMemoryRegion(i, start_address, end_address, type);
    debug(PM, "Ctor: memory region from physical %zx to %zx (%zu bytes) of type %zd\n",
          start_address, end_address, end_address - start_address, type);

    if (type == 1)
      highest_address = Max(highest_address, end_address & 0x7FFFFFFF);
  }

  number_of_pages_ = highest_address / PAGE_SIZE;

  size_t boot_bitmap_size = Min(4096 * 8 * 2, number_of_pages_);
  uint8 page_usage_table[BITMAP_BYTE_COUNT(boot_bitmap_size)];
  used_pages = boot_bitmap_size;
  memset(page_usage_table,0xFF,BITMAP_BYTE_COUNT(boot_bitmap_size));

  //mark as free, everything that might be useable
  for (size_t i = 0; i < num_mmaps; ++i)
  {
    pointer start_address = 0, end_address = 0, type = 0;
    ArchCommon::getUsableMemoryRegion(i, start_address, end_address, type);
    if (type != 1)
      continue;
    size_t start_page = start_address / PAGE_SIZE;
    size_t end_page = end_address / PAGE_SIZE;
    debug(PM, "Ctor: usable memory region: start_page: %zx, end_page: %zx, type: %zd\n", start_page, end_page, type);

    for (size_t k = Max(start_page, lowest_unreserved_page_); k < Min(end_page, number_of_pages_); ++k)
    {
      Bitmap::unsetBit(page_usage_table, used_pages, k);
    }
  }

  debug(PM, "Ctor: Marking pages used by the kernel as reserved\n");
  for (size_t i = ArchMemory::RESERVED_START; i < ArchMemory::RESERVED_END; ++i)
  {
    size_t physical_page = 0;
    size_t pte_page = 0;
    size_t this_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(i, &physical_page, &pte_page);
    assert(this_page_size == 0 || this_page_size == PAGE_SIZE || this_page_size == PAGE_SIZE * PAGE_TABLE_ENTRIES);
    if (this_page_size > 0)
    {
      //our bitmap only knows 4k pages for now
      uint64 num_4kpages = this_page_size / PAGE_SIZE; //should be 1 on 4k pages and 1024 on 4m pages
      for (uint64 p = 0; p < num_4kpages; ++p)
      {
        if (physical_page * num_4kpages + p < number_of_pages_)
          Bitmap::setBit(page_usage_table, used_pages, physical_page * num_4kpages + p);
      }
      i += (num_4kpages - 1); //+0 in most cases

      if (num_4kpages == 1 && i % 1024 == 0 && pte_page < number_of_pages_)
        Bitmap::setBit(page_usage_table, used_pages, pte_page);
    }
  }

  debug(PM, "Ctor: Marking GRUB loaded modules as reserved\n");
  //LastbutNotLeast: Mark Modules loaded by GRUB as reserved (i.e. pseudofs, etc)
  for (size_t i = 0; i < ArchCommon::getNumModules(); ++i)
  {
    size_t start_page = (ArchCommon::getModuleStartAddress(i) & 0x7FFFFFFF) / PAGE_SIZE;
    size_t end_page = (ArchCommon::getModuleEndAddress(i) & 0x7FFFFFFF) / PAGE_SIZE;
    debug(PM, "Ctor: module: start_page: %zx, end_page: %zx\n", start_page, end_page);
    for (size_t k = Min(start_page, number_of_pages_); k <= Min(end_page, number_of_pages_ - 1); ++k)
    {
      Bitmap::setBit(page_usage_table, used_pages, k);
      if (ArchMemory::get_PPN_Of_VPN_In_KernelMapping(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k, nullptr, nullptr) == 0)
        ArchMemory::mapKernelPage(PHYSICAL_TO_VIRTUAL_OFFSET / PAGE_SIZE + k,k);
    }
  }

  size_t num_pages_for_bitmap = (number_of_pages_ / 8) / PAGE_SIZE + 1;
  assert(used_pages < number_of_pages_/2 && "No space for kernel heap!");

  HEAP_PAGES = number_of_pages_/2 - used_pages;
  if (HEAP_PAGES > 1024)
    HEAP_PAGES = 1024 + (HEAP_PAGES - Min(HEAP_PAGES,1024))/8;

  size_t start_vpn = ArchCommon::getFreeKernelMemoryStart() / PAGE_SIZE;
  size_t free_page = 0;
  size_t temp_page_size = 0;
  size_t num_reserved_heap_pages = 0;
  for (num_reserved_heap_pages = 0; num_reserved_heap_pages < num_pages_for_bitmap || temp_page_size != 0 ||
                                    num_reserved_heap_pages < ((DYNAMIC_KMM || (number_of_pages_ < 512)) ? 0 : HEAP_PAGES); ++num_reserved_heap_pages)
  {
    while (!Bitmap::setBit(page_usage_table, used_pages, free_page))
      free_page++;
    if ((temp_page_size = ArchMemory::get_PPN_Of_VPN_In_KernelMapping(start_vpn,nullptr,0)) == 0)
      ArchMemory::mapKernelPage(start_vpn,free_page++);
    start_vpn++;
  }

  extern KernelMemoryManager kmm;
  new (&kmm) KernelMemoryManager(num_reserved_heap_pages,HEAP_PAGES);
  page_usage_table_ = new Bitmap(number_of_pages_);

  for (size_t i = 0; i < boot_bitmap_size; ++i)
  {
    if (Bitmap::getBit(page_usage_table,i))
      page_usage_table_->setBit(i);
  }

  debug(PM, "Ctor: find lowest unreserved page\n");
  for (size_t p = 0; p < number_of_pages_; ++p)
  {
    if (!page_usage_table_->getBit(p))
    {
      lowest_unreserved_page_ = p;
      break;
    }
  }
  debug(PM, "Ctor: Physical pages - free: %zu used: %zu total: %u\n", page_usage_table_->getNumFreeBits(),
        page_usage_table_->getNumBitsSet(), number_of_pages_);
  assert(lowest_unreserved_page_ < number_of_pages_);


  debug(PM, "Clearing free pages\n");
  for(size_t p = lowest_unreserved_page_; p < number_of_pages_; ++p)
  {
    if(!page_usage_table_->getBit(p))
    {
      memset((void*)ArchMemory::getIdentAddressOfPPN(p), 0xFF, PAGE_SIZE);
    }
  }

  num_pages_for_user_ = DYNAMIC_KMM ? -1 : getNumFreePages();
  KernelMemoryManager::pm_ready_ = 1;
}

/**
 * @class PageManager
 *
 * This class represents the PageManager in SWEB. It manages the allocation and freeing
 * of physical pages in the system.
 */
uint32 PageManager::getTotalNumPages() const
{
  return number_of_pages_;
}

/**
 * @brief The PageManager class is responsible for managing physical pages.
 *
 * This class provides methods to retrieve information about the physical pages,
 * allocate and free physical pages, and manage the page usage table.
 */
uint32 PageManager::getNumFreePages() const
{
  return page_usage_table_->getNumFreeBits();
}

/**
 * @class PageManager
 * @brief Manages the allocation and deallocation of physical pages
 */
bool PageManager::reservePages(uint32 ppn, uint32 num)
{
  assert(page_manager_lock_.heldBy() == currentThread);
  if (ppn < number_of_pages_ && !page_usage_table_->getBit(ppn))
  {
    if (num == 1 || reservePages(ppn + 1, num - 1))
    {
      page_usage_table_->setBit(ppn);
      return true;
    }
  }
  return false;
}

/**
 * @class PageManager
 * @brief Manages physical pages in the SWEB operating system
 */
uint32 PageManager::allocPPN(uint32 page_size)
{
  uint32 p;
  uint32 found = 0;
  assert((page_size % PAGE_SIZE) == 0);

  page_manager_lock_.acquire();

  for (p = lowest_unreserved_page_; !found && (p < number_of_pages_); ++p)
  {
    if ((p % (page_size / PAGE_SIZE)) != 0)
      continue;
    if (reservePages(p, page_size / PAGE_SIZE))
      found = p;
  }
  while ((lowest_unreserved_page_ < number_of_pages_) && page_usage_table_->getBit(lowest_unreserved_page_))
    ++lowest_unreserved_page_;

  page_manager_lock_.release();

  if (found == 0)
  {
    assert(false && "PageManager::allocPPN: Out of memory / No more free physical pages");
  }

  const char* page_ident_addr = (const char*)ArchMemory::getIdentAddressOfPPN(found);
  const char* page_modified = (const char*)memnotchr(page_ident_addr, 0xFF, page_size);
  if(page_modified)
  {
    debug(PM, "Detected use-after-free for PPN %x at offset %zx\n", found, page_modified - page_ident_addr);
    assert(!page_modified && "Page modified after free");
  }

  memset((void*)ArchMemory::getIdentAddressOfPPN(found), 0, page_size);
  return found;
}

/**
 * @brief Free a physical page in the page manager
 *
 * This function frees a physical page in the page manager by marking it as unused in the page usage table. If the page number
 * is greater than the total number of pages, an assertion error is thrown. The memory of the page is then set to 0xFF to clear
 * any existing data. The lowest unreserved page is updated if necessary. The function acquires the lock to ensure thread safety.
 *
 * @param page_number The physical page number to free
 * @param page_size The size of the page to free (must be a multiple of PAGE_SIZE)
 */
void PageManager::freePPN(uint32 page_number, uint32 page_size)
{
  assert((page_size % PAGE_SIZE) == 0);
  if(page_number > getTotalNumPages())
  {
    debug(PM, "Tried to free PPN %u (=%x)\n", page_number, page_number);
    assert(false && "PPN to be freed is out of range");
  }
  memset((void*)ArchMemory::getIdentAddressOfPPN(page_number), 0xFF, page_size);

  page_manager_lock_.acquire();
  if (page_number < lowest_unreserved_page_)
    lowest_unreserved_page_ = page_number;
  for (uint32 p = page_number; p < (page_number + page_size / PAGE_SIZE); ++p)
  {
    //debug(FORK, "p %d\n",p);
    //debug(FORK, "page_usage_table_->getBit(p) %d\n",page_usage_table_->getBit(p));
    assert(page_usage_table_->getBit(p) && "Double free PPN");
    page_usage_table_->unsetBit(p);
  }
  page_manager_lock_.release();
}

/**
 * @brief Prints the bitmap representation of the page_usage_table_.
 *
 * This function prints the bitmap representation of the page_usage_table_ to the console.
 * The page_usage_table_ keeps track of the usage status of physical pages.
 */
void PageManager::printBitmap()
{
  page_usage_table_->bmprint();
}

/**
 * @class PageManager
 * The PageManager class is responsible for managing physical pages in SWEB.
 * It provides various methods to allocate and free physical pages.
 */
uint32 PageManager::getNumPagesForUser() const
{
  return num_pages_for_user_;
}

void PageManager::incrementReferenceCount(uint64 page_number)
{
  assert(page_reference_counts_lock_.heldBy() == currentThread);
  //check if the page number is already in the map
  auto it = page_reference_counts_.find(page_number);
  if (it != page_reference_counts_.end())
  {
    //page number found, increment
    page_reference_counts_[page_number]++;
  }
  else
  {
    //page number not found, initialize reference count to 1
    page_reference_counts_[page_number] = 1;
  }
}

void PageManager::decrementReferenceCount(uint64 page_number)
{
  assert(page_reference_counts_lock_.heldBy() == currentThread);
  //check if the page number is in the map
  auto it = page_reference_counts_.find(page_number);
  if (it != page_reference_counts_.end())
  {
    //decrement the reference count
    page_reference_counts_[page_number]--;

    //if reference count reaches zero, erase the entry from the map
    if (it->second == 0)
    {
      page_reference_counts_.erase(it);
    }
  }
}

uint32 PageManager::getReferenceCount(uint64 page_number)
{
  assert(page_reference_counts_lock_.heldBy() == currentThread);
  // Check if the page number is in the map
  auto it = page_reference_counts_.find(static_cast<uint32>(page_number));
  if (it != page_reference_counts_.end())
  {
    return it->second;
  }
  else
  {
    return 0;
  }
}


// void PageManager::insertInvertedPageTable(uint64 ppn, PageTableEntry* pte)
// {
//   debug(SWAPPING, "PageManager::insertInvertedPageTable: ppn: %zx, pte: %zx\n", ppn, pte);
//   inverted_page_table_lock_.acquire();
//   inverted_page_table_[ppn].push_back(pte);
//   inverted_page_table_lock_.release();
// }

// void PageManager::removeFromInvertedPageTable(uint64 ppn, PageTableEntry* pte)
// {
//   debug(SWAPPING, "PageManager::removeFromInvertedPageTable: ppn: %zx, pte: %zx\n", ppn, pte);
//   inverted_page_table_lock_.acquire();
//   auto it = inverted_page_table_.find(ppn);
//   if (it != inverted_page_table_.end())
//   {
//     auto& vec = it->second;
//     auto vec_it = ustl::find(vec.begin(), vec.end(), pte);
//     if (vec_it != vec.end())
//     {
//       vec.erase(vec_it);
//     }
//     if (vec.empty())
//     {
//       inverted_page_table_.erase(it);
//     }
//   }
//   inverted_page_table_lock_.release();
// }
