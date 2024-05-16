#include "ArchMemory.h"
#include "ArchInterrupts.h"
#include "kprintf.h"
#include "PageManager.h"
#include "kstring.h"
#include "ArchThreads.h"
#include "Thread.h"
#include "UserThread.h"
#include "UserProcess.h"
#include "assert.h"

PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirPointerTableEntry kernel_page_directory_pointer_table[2 * PAGE_DIR_POINTER_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirEntry kernel_page_directory[2 * PAGE_DIR_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageTableEntry kernel_page_table[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));


ArchMemory::ArchMemory():lock_("archmemory_lock_")
{
  lock_.acquire();
  page_map_level_4_ = PageManager::instance()->allocPPN();
  PageMapLevel4Entry* new_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  memcpy((void*) new_pml4, (void*) kernel_page_map_level_4, PAGE_SIZE);
  memset(new_pml4, 0, PAGE_SIZE / 2); // should be zero, this is just for safety, also only clear lower half
  lock_.release();
}

// COPY CONSTRUCTOR
ArchMemory::ArchMemory(ArchMemory const &src):lock_("archmemory_lock_")
{
  assert(((UserThread*) currentThread)->process_->loader_->arch_memory_.lock_.isHeldBy((Thread*) currentThread) && "The parent's archmem is not locked");
  lock_.acquire();
  assert(PageManager::instance()->heldBy() != currentThread);

  debug(FORK, "ArchMemory::copy-constructor starts \n");
  page_map_level_4_ = PageManager::instance()->allocPPN();
  PageMapLevel4Entry* CHILD_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  PageMapLevel4Entry* PARENT_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(src.page_map_level_4_);
  memcpy((void*) CHILD_pml4, (void*) PARENT_pml4, PAGE_SIZE);
  // memset(CHILD_pml4, 0, PAGE_SIZE / 2); // should be zero already, this is just for safety, also only clear lower half (User half)

  debug(FORK, "copy-ctor start copying all pages\n");
  // Loop through the pml4 to get each pdpt
  for (uint64 pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++) // copy only lower half (userspace)
  {
    if (PARENT_pml4[pml4i].present)
    {
      // setup new page directory pointer table
      CHILD_pml4[pml4i].page_ppn = PageManager::instance()->allocPPN();

      //CHILD_pml4[pml4i].page_ppn = PARENT_pml4[pml4i].page_ppn;
      PageDirPointerTableEntry* CHILD_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(CHILD_pml4[pml4i].page_ppn);
      PageDirPointerTableEntry* PARENT_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(PARENT_pml4[pml4i].page_ppn);
      memcpy((void*) CHILD_pdpt, (void*) PARENT_pdpt, PAGE_SIZE);

      // debug(FORK, "PARENT_pml4[pml4i].present: %ld\n",PARENT_pml4[pml4i].present);
      // debug(FORK, "CHILD_pml4[pml4i].present: %ld\n",CHILD_pml4[pml4i].present);
      assert(CHILD_pml4[pml4i].present == 1 && "The page map level 4 entries should be both be present in child and parent");

      // loop through pdpt to get each pd
      for (uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
      {
        if (PARENT_pdpt[pdpti].pd.present)
        {
          // setup new page directory
          CHILD_pdpt[pdpti].pd.page_ppn = PageManager::instance()->allocPPN();

          //CHILD_pdpt[pdpti].pd.page_ppn = PARENT_pdpt[pdpti].pd.page_ppn;
          PageDirEntry* CHILD_pd = (PageDirEntry*) getIdentAddressOfPPN(CHILD_pdpt[pdpti].pd.page_ppn);
          PageDirEntry* PARENT_pd = (PageDirEntry*) getIdentAddressOfPPN(PARENT_pdpt[pdpti].pd.page_ppn);
          memcpy((void*) CHILD_pd, (void*) PARENT_pd, PAGE_SIZE);

          // debug(FORK, "PARENT_pdpt[pdpti].pd.present: %ld\n",PARENT_pdpt[pdpti].pd.present);
          // debug(FORK, "CHILD_pdpt[pdpti].pd.present: %ld\n",CHILD_pdpt[pdpti].pd.present);
          assert(CHILD_pdpt[pdpti].pd.present == 1 && "The page directory pointer table entries should be both be present in child and parent");

          // loop through pd to get each pt
          for (uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
          {
            if (PARENT_pd[pdi].pt.present)
            {
              // setup new page table
              CHILD_pd[pdi].pt.page_ppn = PageManager::instance()->allocPPN();

              //CHILD_pd[pdi].pt.page_ppn = PARENT_pd[pdi].pt.page_ppn;
              PageTableEntry* CHILD_pt = (PageTableEntry*) getIdentAddressOfPPN(CHILD_pd[pdi].pt.page_ppn);
              PageTableEntry* PARENT_pt = (PageTableEntry*) getIdentAddressOfPPN(PARENT_pd[pdi].pt.page_ppn);
              memcpy((void*) CHILD_pt, (void*) PARENT_pt, PAGE_SIZE);

              // debug(FORK, "PARENT_pd[pdi].pt.present: %ld\n",PARENT_pd[pdi].pt.present);
              // debug(FORK, "CHILD_pd[pdi].pt.present: %ld\n",CHILD_pd[pdi].pt.present);
              assert(CHILD_pd[pdi].pt.present == 1 && "The page directory entries should be both be present in child and parent");

              // loop through pt to get each pageT
              for (uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
              {
                if (PARENT_pt[pti].present)
                {
                  //CHILD_pt[pti].page_ppn = PageManager::instance()->allocPPN();
                  //CHILD_pt[pti].page_ppn = PARENT_pt[pti].page_ppn;

                  PARENT_pt[pti].writeable = 0; //read only
                  PARENT_pt[pti].cow = 1;

                  CHILD_pt[pti].writeable = 0; //read only
                  CHILD_pt[pti].cow = 1;

                  PageManager::instance()->page_reference_counts_lock_.acquire();
                  PageManager::instance()->incrementReferenceCount(PARENT_pt[pti].page_ppn);
                  assert(PageManager::instance()->getReferenceCount(PARENT_pt[pti].page_ppn) >= 2 && "The reference count should be at least 2");

                  debug(FORK, "getReferenceCount in copyconstructor child: %d, parent: %d \n", PageManager::instance()->getReferenceCount(CHILD_pt[pti].page_ppn),
                                                                                               PageManager::instance()->getReferenceCount(PARENT_pt[pti].page_ppn));
                  PageManager::instance()->page_reference_counts_lock_.release();

                  assert(CHILD_pt[pti].present == 1 && "The page directory entries should be both be present in child and parent");
                }
              }
            }
          }
        }
      }
    }
  }
  debug(FORK, "ArchMemory::copy-constructor finished \n");
  lock_.release();
}


ArchMemory::~ArchMemory()
{
  debug(FORK, "~ArchMemory \n");
  lock_.acquire();
  assert(currentThread->kernel_registers_->cr3 != page_map_level_4_ * PAGE_SIZE && "thread deletes its own arch memory");

  PageMapLevel4Entry* pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  for (uint64 pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++) // free only lower half
  {
    if (pml4[pml4i].present)
    {
      PageDirPointerTableEntry* pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[pml4i].page_ppn);
      for (uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
      {
        if (pdpt[pdpti].pd.present)
        {
          assert(pdpt[pdpti].pd.size == 0);
          PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[pdpti].pd.page_ppn);
          for (uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
          {
            if (pd[pdi].pt.present)
            {
              assert(pd[pdi].pt.size == 0);
              PageTableEntry* pt = (PageTableEntry*) getIdentAddressOfPPN(pd[pdi].pt.page_ppn);
              for (uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
              {
                if (pt[pti].present)
                {
                  PageManager::instance()->page_reference_counts_lock_.acquire();
                  if(PageManager::instance()->getReferenceCount(pt[pti].page_ppn) == 1)
                  {
                    debug(FORK, "free page and set present in destructor  \n");
                    pt[pti].present = 0;
                    PageManager::instance()->freePPN(pt[pti].page_ppn);
                    PageManager::instance()->decrementReferenceCount(pt[pti].page_ppn);
                    debug(FORK, "getReferenceCount in destructor (free) %d \n", PageManager::instance()->getReferenceCount(pt[pti].page_ppn));
                  }
                  else
                  {
                    pt[pti].present = 0;
                    PageManager::instance()->decrementReferenceCount(pt[pti].page_ppn);
                    debug(FORK, "getReferenceCount in destructor (decrement) %d \n", PageManager::instance()->getReferenceCount(pt[pti].page_ppn));
                  }
                  PageManager::instance()->page_reference_counts_lock_.release();
                }
              }
              pd[pdi].pt.present = 0;
              PageManager::instance()->freePPN(pd[pdi].pt.page_ppn);
            }
          }
          pdpt[pdpti].pd.present = 0;
          PageManager::instance()->freePPN(pdpt[pdpti].pd.page_ppn);
        }
      }
      pml4[pml4i].present = 0;
      PageManager::instance()->freePPN(pml4[pml4i].page_ppn);
    }
  }
  PageManager::instance()->freePPN(page_map_level_4_);
  lock_.release();
}

pointer ArchMemory::checkAddressValid(uint64 vaddress_to_check)
{
  ArchMemoryMapping m = resolveMapping(page_map_level_4_, vaddress_to_check / PAGE_SIZE);
  if (m.page != 0)
  {
    debug(A_MEMORY, "checkAddressValid %zx and %zx -> true\n", page_map_level_4_, vaddress_to_check);
    return m.page | (vaddress_to_check % m.page_size);
  }
  else
  {
    debug(A_MEMORY, "checkAddressValid %zx and %zx -> false\n", page_map_level_4_, vaddress_to_check);
    return 0;
  }
}


template<typename T>
bool ArchMemory::checkAndRemove(pointer map_ptr, uint64 index)
{
  T* map = (T*) map_ptr;
  debug(A_MEMORY, "%s: page %p index %zx\n", __PRETTY_FUNCTION__, map, index);
  ((uint64*) map)[index] = 0;
  for (uint64 i = 0; i < PAGE_DIR_ENTRIES; i++)
  {
    if (map[i].present != 0)
      return false;
  }
  return true;
}

bool ArchMemory::unmapPage(uint64 virtual_page)
{
  assert(lock_.heldBy() == currentThread && "Try to unmap page without holding archmemory lock");
  ArchMemoryMapping m = resolveMapping(virtual_page);

  assert(m.page_ppn != 0);
  assert(m.page_size == PAGE_SIZE);
  assert(m.pt[m.pti].present);
  m.pt[m.pti].present = 0;

  PageManager::instance()->page_reference_counts_lock_.acquire();
  if(PageManager::instance()->getReferenceCount(m.page_ppn) == 1)
  {
    PageManager::instance()->decrementReferenceCount(m.page_ppn);
    debug(FORK, "getReferenceCount in unmapPage %d Page:%ld (free)\n", PageManager::instance()->getReferenceCount(m.page_ppn), (m.page_ppn));
    PageManager::instance()->freePPN(m.page_ppn);
    PageManager::instance()->page_reference_counts_lock_.release();
  }
  else
  {
    PageManager::instance()->decrementReferenceCount(m.page_ppn);
    debug(FORK, "getReferenceCount in unmapPage %d Page:%ld (decrease)\n", PageManager::instance()->getReferenceCount(m.page_ppn), (m.page_ppn));
    PageManager::instance()->page_reference_counts_lock_.release();
    return true;
  }


  ((uint64*)m.pt)[m.pti] = 0; // for easier debugging
  bool empty = checkAndRemove<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti);

  if (empty)
  {
    empty = checkAndRemove<PageDirPageTableEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi);
    PageManager::instance()->freePPN(m.pt_ppn);
  }
  if (empty)
  {
    empty = checkAndRemove<PageDirPointerTablePageDirEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdpti);
    PageManager::instance()->freePPN(m.pd_ppn);
  }
  if (empty)
  {
    checkAndRemove<PageMapLevel4Entry>(getIdentAddressOfPPN(m.pml4_ppn), m.pml4i);
    PageManager::instance()->freePPN(m.pdpt_ppn);
  }
  return true;
}

template<typename T>
void ArchMemory::insert(pointer map_ptr, uint64 index, uint64 ppn, uint64 bzero, uint64 size, uint64 user_access,
                        uint64 writeable)
{
  assert(map_ptr & ~0xFFFFF00000000000ULL);
  T* map = (T*) map_ptr;
  debug(A_MEMORY, "%s: page %p index %zx ppn %zx user_access %zx size %zx\n", __PRETTY_FUNCTION__, map, index, ppn,
        user_access, size);
  if (bzero)
  {
    memset((void*) getIdentAddressOfPPN(ppn), 0, PAGE_SIZE);
    assert(((uint64* )map)[index] == 0);
  }
  map[index].size = size;
  map[index].writeable = writeable;
  map[index].page_ppn = ppn;
  map[index].user_access = user_access;
  map[index].present = 1;
}

bool ArchMemory::mapPage(uint64 virtual_page, uint64 physical_page, uint64 user_access)
{
  assert(PageManager::instance()->heldBy() != currentThread && "Holding pagemanager lock when mapPage can lead to double locking.");
  assert(lock_.heldBy() == currentThread && "Try to map page without holding archmemory lock");
  ArchMemoryMapping m = resolveMapping(page_map_level_4_, virtual_page);

  assert((m.page_size == 0) || (m.page_size == PAGE_SIZE));

  if (m.pdpt_ppn == 0)
  {
    m.pdpt_ppn = PageManager::instance()->allocPPN();
    insert<PageMapLevel4Entry>((pointer) m.pml4, m.pml4i, m.pdpt_ppn, 1, 0, 1, 1);
  }

  if (m.pd_ppn == 0)
  {
    m.pd_ppn = PageManager::instance()->allocPPN();
    insert<PageDirPointerTablePageDirEntry>(getIdentAddressOfPPN(m.pdpt_ppn), m.pdpti, m.pd_ppn, 1, 0, 1, 1);
  }

  if (m.pt_ppn == 0)
  {
    m.pt_ppn = PageManager::instance()->allocPPN();
    insert<PageDirPageTableEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi, m.pt_ppn, 1, 0, 1, 1);
  }

  if (m.page_ppn == 0)
  {
    insert<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti, physical_page, 0, 0, user_access, 1);
    uint64 page_ppn = ((PageTableEntry*)getIdentAddressOfPPN(m.pt_ppn))[m.pti].page_ppn;
    PageManager::instance()->page_reference_counts_lock_.acquire();
    PageManager::instance()->incrementReferenceCount(page_ppn);
    debug(FORK, "getReferenceCount in mappage %d %ld \n", PageManager::instance()->getReferenceCount(page_ppn), (page_ppn));
    PageManager::instance()->page_reference_counts_lock_.release();
    return true;
  }
  return false;
}




const ArchMemoryMapping ArchMemory::resolveMapping(uint64 vpage)
{
  assert(lock_.heldBy() == currentThread && "Try to resolve mapping without holding archmemory lock");
  return resolveMapping(page_map_level_4_, vpage);
}

const ArchMemoryMapping ArchMemory::resolveMapping(uint64 pml4, uint64 vpage)
{
  assert((vpage * PAGE_SIZE < USER_BREAK || vpage * PAGE_SIZE >= KERNEL_START) &&
         "This is not a valid vpn! Did you pass an address to resolveMapping?");
  ArchMemoryMapping m;

  m.pti = vpage;
  m.pdi = m.pti / PAGE_TABLE_ENTRIES;
  m.pdpti = m.pdi / PAGE_DIR_ENTRIES;
  m.pml4i = m.pdpti / PAGE_DIR_POINTER_TABLE_ENTRIES;

  m.pti %= PAGE_TABLE_ENTRIES;
  m.pdi %= PAGE_DIR_ENTRIES;
  m.pdpti %= PAGE_DIR_POINTER_TABLE_ENTRIES;
  m.pml4i %= PAGE_MAP_LEVEL_4_ENTRIES;

  assert(pml4 < PageManager::instance()->getTotalNumPages());
  m.pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(pml4);
  m.pdpt = 0;
  m.pd = 0;
  m.pt = 0;
  m.page = 0;
  m.pml4_ppn = pml4;
  m.pdpt_ppn = 0;
  m.pd_ppn = 0;
  m.pt_ppn = 0;
  m.page_ppn = 0;
  m.page_size = 0;
  if (m.pml4[m.pml4i].present)
  {
    m.pdpt_ppn = m.pml4[m.pml4i].page_ppn;
    m.pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(m.pml4[m.pml4i].page_ppn);
    if (m.pdpt[m.pdpti].pd.present && !m.pdpt[m.pdpti].pd.size) // 1gb page ?
    {
      m.pd_ppn = m.pdpt[m.pdpti].pd.page_ppn;
      if (m.pd_ppn > PageManager::instance()->getTotalNumPages())
      {
        debug(A_MEMORY, "%zx\n", m.pd_ppn);
      }
      assert(m.pd_ppn < PageManager::instance()->getTotalNumPages());
      m.pd = (PageDirEntry*) getIdentAddressOfPPN(m.pdpt[m.pdpti].pd.page_ppn);
      if (m.pd[m.pdi].pt.present && !m.pd[m.pdi].pt.size) // 2mb page ?
      {
        m.pt_ppn = m.pd[m.pdi].pt.page_ppn;
        assert(m.pt_ppn < PageManager::instance()->getTotalNumPages());
        m.pt = (PageTableEntry*) getIdentAddressOfPPN(m.pd[m.pdi].pt.page_ppn);
        if (m.pt[m.pti].present)
        {
          m.page = getIdentAddressOfPPN(m.pt[m.pti].page_ppn);
          m.page_ppn = m.pt[m.pti].page_ppn;
          assert(m.page_ppn < PageManager::instance()->getTotalNumPages());
          m.page_size = PAGE_SIZE;
        }
      }
      else if (m.pd[m.pdi].page.present)
      {
        m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES;
        m.page_ppn = m.pd[m.pdi].page.page_ppn;
        m.page = getIdentAddressOfPPN(m.pd[m.pdi].page.page_ppn);
      }
    }
    else if (m.pdpt[m.pdpti].page.present)
    {
      m.page_size = PAGE_SIZE * PAGE_TABLE_ENTRIES * PAGE_DIR_ENTRIES;
      m.page_ppn = m.pdpt[m.pdpti].page.page_ppn;
      assert(m.page_ppn < PageManager::instance()->getTotalNumPages());
      m.page = getIdentAddressOfPPN(m.pdpt[m.pdpti].page.page_ppn);
    }
  }
  return m;
}

uint64 ArchMemory::get_PPN_Of_VPN_In_KernelMapping(size_t virtual_page, size_t *physical_page,
                                                   size_t *physical_pte_page)
{
  ArchMemoryMapping m = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE),
                                       virtual_page);
  if (physical_page)
    *physical_page = m.page_ppn;
  if (physical_pte_page)
    *physical_pte_page = m.pt_ppn;
  return m.page_size;
}

void ArchMemory::mapKernelPage(size_t virtual_page, size_t physical_page)
{
  ArchMemoryMapping mapping = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE),
                                             virtual_page);
  PageMapLevel4Entry* pml4 = kernel_page_map_level_4;
  assert(pml4[mapping.pml4i].present);
  PageDirPointerTableEntry *pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[mapping.pml4i].page_ppn);
  assert(pdpt[mapping.pdpti].pd.present);
  PageDirEntry *pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[mapping.pdpti].pd.page_ppn);
  assert(pd[mapping.pdi].pt.present);
  PageTableEntry *pt = (PageTableEntry*) getIdentAddressOfPPN(pd[mapping.pdi].pt.page_ppn);
  assert(!pt[mapping.pti].present);
  pt[mapping.pti].writeable = 1;
  pt[mapping.pti].page_ppn = physical_page;
  pt[mapping.pti].present = 1;
  asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

void ArchMemory::unmapKernelPage(size_t virtual_page)
{
  debug(FORK, "unmapKernelPage \n");
  ArchMemoryMapping mapping = resolveMapping(((uint64) VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4) / PAGE_SIZE),
                                             virtual_page);
  PageMapLevel4Entry* pml4 = kernel_page_map_level_4;
  assert(pml4[mapping.pml4i].present);
  PageDirPointerTableEntry *pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[mapping.pml4i].page_ppn);
  assert(pdpt[mapping.pdpti].pd.present);
  PageDirEntry *pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[mapping.pdpti].pd.page_ppn);
  assert(pd[mapping.pdi].pt.present);
  PageTableEntry *pt = (PageTableEntry*) getIdentAddressOfPPN(pd[mapping.pdi].pt.page_ppn);
  assert(pt[mapping.pti].present);
  pt[mapping.pti].present = 0;
  pt[mapping.pti].writeable = 0;
  PageManager::instance()->freePPN(pt[mapping.pti].page_ppn);
  asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

PageMapLevel4Entry* ArchMemory::getRootOfKernelPagingStructure()
{
  return kernel_page_map_level_4;
}


void ArchMemory::deleteEverythingExecpt(size_t virtual_page)
{
  lock_.acquire();
  ArchMemoryMapping m = resolveMapping(page_map_level_4_, virtual_page);

  PageMapLevel4Entry* pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  for (uint64 pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++) // free only lower half
  {
    if (pml4[pml4i].present)
    {
      PageDirPointerTableEntry* pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(pml4[pml4i].page_ppn);
      for (uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
      {
        if (pdpt[pdpti].pd.present)
        {
          assert(pdpt[pdpti].pd.size == 0);
          PageDirEntry* pd = (PageDirEntry*) getIdentAddressOfPPN(pdpt[pdpti].pd.page_ppn);
          for (uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
          {
            if (pd[pdi].pt.present)
            {
              assert(pd[pdi].pt.size == 0);
              PageTableEntry* pt = (PageTableEntry*) getIdentAddressOfPPN(pd[pdi].pt.page_ppn);
              for (uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
              {
                if (pt[pti].present)
                {
                  if(m.page_ppn != pt[pti].page_ppn)
                  {
                    PageManager::instance()->page_reference_counts_lock_.acquire();
                    if(PageManager::instance()->getReferenceCount(pt[pti].page_ppn) == 1)
                    {
                      PageManager::instance()->freePPN(pt[pti].page_ppn);
                      PageManager::instance()->decrementReferenceCount(pt[pti].page_ppn);
                      debug(FORK, "getReferenceCount in exec_destructor (free) %d \n", PageManager::instance()->getReferenceCount(pt[pti].page_ppn));
                      ((uint64*)pt)[pti] = 0;
                    }
                    else
                    {
                      PageManager::instance()->decrementReferenceCount(pt[pti].page_ppn);
                      debug(FORK, "getReferenceCount in exec_destructor (decrement) %d \n", PageManager::instance()->getReferenceCount(pt[pti].page_ppn));
                      ((uint64*)pt)[pti] = 0;
                    }
                    PageManager::instance()->page_reference_counts_lock_.release();
                  }
                }
              }
              if(m.pt_ppn != pd[pdi].pt.page_ppn)
              {
                pd[pdi].pt.present = 0;
                PageManager::instance()->freePPN(pd[pdi].pt.page_ppn);
                ((uint64*)pd)[pdi] = 0; 
              }
            }
          }
          if(m.pd_ppn != pdpt[pdpti].pd.page_ppn)
          {
            pdpt[pdpti].pd.present = 0;
            PageManager::instance()->freePPN(pdpt[pdpti].pd.page_ppn);
            ((uint64*)pdpt)[pdpti] = 0; 
          }
        }
      }
      if(m.pdpt_ppn != pml4[pml4i].page_ppn)
      {
        pml4[pml4i].present = 0;
        PageManager::instance()->freePPN(pml4[pml4i].page_ppn);
        ((uint64*)pml4)[pml4i] = 0; 
      }
    }
  }
  lock_.release();
}


bool ArchMemory::isCOW(size_t virtual_addr)
{
  ArchMemoryMapping pml1 = ArchMemory::resolveMapping(virtual_addr/PAGE_SIZE);
  PageTableEntry* pml1_entry = &pml1.pt[pml1.pti];

  if (pml1_entry && pml1_entry->cow)
    return true;
  else
    return false;
}


void ArchMemory::copyPage(size_t virtual_addr)
{
  PageManager* pm = PageManager::instance();

  debug(FORK, "ArchMemory::copyPage Resolving mapping \n");
  ArchMemoryMapping pml1 = ArchMemory::resolveMapping(virtual_addr/PAGE_SIZE);
  PageTableEntry* pml1_entry = &pml1.pt[pml1.pti];
  assert(pml1_entry && pml1_entry->cow && "Page is not COW");

  debug(FORK, "ArchMemory::copyPage If the process is not the last process to own the page, then copy to new page\n");
  pm->page_reference_counts_lock_.acquire();
  assert(pm->getReferenceCount(pml1_entry->page_ppn) > 0 && "Reference count is 0");
  if (pm->getReferenceCount(pml1_entry->page_ppn) > 1)
  {
    debug(FORK, "ArchMemory::copyPage: Ref count is > 1, Copying to a new page\n");
    size_t new_page_ppn = pm->allocPPN();
    pointer original_page = ArchMemory::getIdentAddressOfPPN(pml1_entry->page_ppn);
    pointer new_page = ArchMemory::getIdentAddressOfPPN(new_page_ppn);
    memcpy((void*)new_page, (void*)original_page, PAGE_SIZE);

    debug(FORK, "ArchMemory::copyPage update the ref count for old page and new page\n");
    pm->decrementReferenceCount(pml1_entry->page_ppn);
    pml1_entry->page_ppn = new_page_ppn;
    pm->incrementReferenceCount(pml1_entry->page_ppn);
  }
  pm->page_reference_counts_lock_.release();

  debug(FORK, "ArchMemory::copyPage Setting up the bit of the new page (present, write, !cow)\n");
  // pml1_entry->present = 1;   // this should already be 1
  pml1_entry->writeable = 1;
  pml1_entry->cow = 0;  
}