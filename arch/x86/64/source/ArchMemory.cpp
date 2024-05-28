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
#include "SwappingManager.h"


PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirPointerTableEntry kernel_page_directory_pointer_table[2 * PAGE_DIR_POINTER_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageDirEntry kernel_page_directory[2 * PAGE_DIR_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
PageTableEntry kernel_page_table[8 * PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));


ArchMemory::ArchMemory():archmemory_lock_("archmemory_lock_")
{
  IPTManager::instance()->IPT_lock_.acquire();
  archmemory_lock_.acquire();
  page_map_level_4_ = PageManager::instance()->allocPPN();
  PageMapLevel4Entry* new_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  memcpy((void*) new_pml4, (void*) kernel_page_map_level_4, PAGE_SIZE);
  memset(new_pml4, 0, PAGE_SIZE / 2); // should be zero, this is just for safety, also only clear lower half
  archmemory_lock_.release();
  IPTManager::instance()->IPT_lock_.release();
}

// COPY CONSTRUCTOR
ArchMemory::ArchMemory(ArchMemory const &src):archmemory_lock_("archmemory_lock_")
{
  assert(src.archmemory_lock_.heldBy() == currentThread);
  assert(PageManager::instance()->heldBy() != currentThread);
  archmemory_lock_.acquire();
  
  debug(FORK, "ArchMemory::copy-constructor starts \n");
  page_map_level_4_ = PageManager::instance()->allocPPN();
  PageMapLevel4Entry* CHILD_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(page_map_level_4_);
  PageMapLevel4Entry* PARENT_pml4 = (PageMapLevel4Entry*) getIdentAddressOfPPN(src.page_map_level_4_);
  memcpy((void*) CHILD_pml4, (void*) PARENT_pml4, PAGE_SIZE);


  debug(FORK, "copy-ctor start copying all pages\n");
  // Loop through the pml4 to get each pdpt
  for (uint64 pml4i = 0; pml4i < PAGE_MAP_LEVEL_4_ENTRIES / 2; pml4i++) // copy only lower half (userspace)
  {
    if (PARENT_pml4[pml4i].present)
    {
      //debug(COW, "---------------PageFault PML4 Loop \n");
      PARENT_pml4[pml4i].cow = 1;
      //PARENT_pml4[pml4i].writeable = 0;

      CHILD_pml4[pml4i].cow = 1;
      //CHILD_pml4[pml4i].writeable = 0;

      CHILD_pml4[pml4i].page_ppn = PageManager::instance()->allocPPN();


      PageDirPointerTableEntry* CHILD_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(CHILD_pml4[pml4i].page_ppn);
      PageDirPointerTableEntry* PARENT_pdpt = (PageDirPointerTableEntry*) getIdentAddressOfPPN(PARENT_pml4[pml4i].page_ppn);
      memcpy((void*) CHILD_pdpt, (void*) PARENT_pdpt, PAGE_SIZE);

      assert(CHILD_pml4[pml4i].present == 1 && "The page map level 4 entries should be both be present in child and parent");

      // loop through pdpt to get each pd
      for (uint64 pdpti = 0; pdpti < PAGE_DIR_POINTER_TABLE_ENTRIES; pdpti++)
      {
        if (PARENT_pdpt[pdpti].pd.present)
        {
          //debug(COW, "---------------PageFault PML3 Loop \n");
          PARENT_pdpt[pdpti].pd.cow = 1;
          //PARENT_pdpt[pdpti].pd.writeable = 0;

          CHILD_pdpt[pdpti].pd.cow = 1;
          //CHILD_pdpt[pdpti].pd.writeable = 0;

          CHILD_pdpt[pdpti].pd.page_ppn = PageManager::instance()->allocPPN();

          //CHILD_pdpt[pdpti].pd.page_ppn = PARENT_pdpt[pdpti].pd.page_ppn;
          PageDirEntry* CHILD_pd = (PageDirEntry*) getIdentAddressOfPPN(CHILD_pdpt[pdpti].pd.page_ppn);
          PageDirEntry* PARENT_pd = (PageDirEntry*) getIdentAddressOfPPN(PARENT_pdpt[pdpti].pd.page_ppn);
          memcpy((void*) CHILD_pd, (void*) PARENT_pd, PAGE_SIZE);
          assert(CHILD_pdpt[pdpti].pd.present == 1 && "The page directory pointer table entries should be both be present in child and parent");

          // loop through pd to get each pt
          for (uint64 pdi = 0; pdi < PAGE_DIR_ENTRIES; pdi++)
          {
            if (PARENT_pd[pdi].pt.present)
            {
              debug(COW, "---------------PageFault PML2 Loop \n");
              PARENT_pd[pdi].pt.cow = 1;
              PARENT_pd[pdi].pt.writeable = 0;
              CHILD_pd[pdi].pt.cow = 1;
              CHILD_pd[pdi].pt.writeable = 0;

              PageManager::instance()->ref_count_lock_.acquire();
              PageManager::instance()->incrementEntryReferenceCount(PARENT_pd[pdi].pt.page_ppn);
              debug(COW, "-------------getReferenceCount %d \n", PageManager::instance()->getReferenceCount(PARENT_pd[pdi].pt.page_ppn));
              PageManager::instance()->ref_count_lock_.release();

              assert(CHILD_pd[pdi].pt.present == 1 && "The page directory entries should be both be present in child and parent");
            }
          }
        }
      }
    }
  }
  debug(FORK, "ArchMemory::copy-constructor finished \n");
  archmemory_lock_.release();
}


ArchMemory::~ArchMemory()
{
  debug(FORK, "ArchMemory::~ArchMemory\n");
  IPTManager::instance()->IPT_lock_.acquire();
  archmemory_lock_.acquire();
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
                  PageManager::instance()->ref_count_lock_.acquire();
                  pt[pti].present = 0;

                  size_t vpn = construct_VPN(pti, pdi, pdpti, pml4i);
                  IPTMapType maptype = getMapType(pt[pti]);

                  PageManager::instance()->decrementReferenceCount(pt[pti].page_ppn, vpn, this, maptype);
                  //debug(FORK, "getReferenceCount in destructor (decrement) %d \n", PageManager::instance()->getReferenceCount(pt[pti].page_ppn));
                  PageManager::instance()->ref_count_lock_.release();
                }
              }
                PageManager::instance()->ref_count_lock_.acquire();
                pd[pdi].pt.present = 0;
                PageManager::instance()->decrementEntryReferenceCount(pd[pdi].pt.page_ppn);
                PageManager::instance()->ref_count_lock_.release();
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
  archmemory_lock_.release();
  IPTManager::instance()->IPT_lock_.release();
  debug(COW, "------------------Arch de finished \n");
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
  assert(archmemory_lock_.heldBy() == currentThread && "Try to unmap page without holding archmemory lock");
  ArchMemoryMapping m = resolveMapping(virtual_page);

  assert(m.page_ppn != 0);
  assert(m.page_size == PAGE_SIZE);
  assert(m.pt[m.pti].present);
  m.pt[m.pti].present = 0;

  IPTMapType maptype = getMapType((m.pt[m.pti]));

  PageManager::instance()->ref_count_lock_.acquire();
  PageManager::instance()->decrementReferenceCount(m.page_ppn, virtual_page, this, maptype);
  debug(FORK, "getReferenceCount in unmapPage %d Page:%ld\n", PageManager::instance()->getReferenceCount(m.page_ppn), (m.page_ppn));
  
  if(PageManager::instance()->getReferenceCount(m.page_ppn) > 0)
  {
    PageManager::instance()->ref_count_lock_.release();
    return true;
  }

  PageManager::instance()->ref_count_lock_.release();



  ((uint64*)m.pt)[m.pti] = 0; // for easier debugging
  bool empty = checkAndRemove<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti);

  if (empty)
  {
    //TODO check ref count == 1 here
    empty = checkAndRemove<PageDirPageTableEntry>(getIdentAddressOfPPN(m.pd_ppn), m.pdi);
    PageManager::instance()->ref_count_lock_.acquire();
    PageManager::instance()->decrementEntryReferenceCount(m.pt_ppn);
    PageManager::instance()->ref_count_lock_.release();
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
  assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread && "IPT need to be alredy  locked.");
  assert(PageManager::instance()->heldBy() != currentThread && "Holding pagemanager lock when mapPage can lead to double locking.");
  assert(archmemory_lock_.heldBy() == currentThread && "Try to map page without holding archmemory lock");


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

    PageManager::instance()->ref_count_lock_.acquire();
    PageManager::instance()->incrementEntryReferenceCount(m.pt_ppn);
    PageManager::instance()->ref_count_lock_.release();
  }

  if (m.page_ppn == 0)
  {
    insert<PageTableEntry>(getIdentAddressOfPPN(m.pt_ppn), m.pti, physical_page, 0, 0, user_access, 1);
    uint64 page_ppn = ((PageTableEntry*)getIdentAddressOfPPN(m.pt_ppn))[m.pti].page_ppn;

    PageManager::instance()->ref_count_lock_.acquire();

    IPTMapType maptype = getMapType(((PageTableEntry*)getIdentAddressOfPPN(m.pt_ppn))[m.pti]);
    PageManager::instance()->incrementReferenceCount(page_ppn, virtual_page, this, maptype);

    debug(FORK, "getReferenceCount in mappage %d %ld \n", PageManager::instance()->getReferenceCount(page_ppn), (page_ppn));
    PageManager::instance()->ref_count_lock_.release();

    return true;
  }
  return false;
}




const ArchMemoryMapping ArchMemory::resolveMapping(uint64 vpage)
{
  assert(archmemory_lock_.heldBy() == currentThread && "Try to resolve mapping without holding archmemory lock");
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
  IPTManager::instance()->IPT_lock_.acquire();
  archmemory_lock_.acquire();
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
                    PageManager::instance()->ref_count_lock_.acquire();

                    size_t vpn = construct_VPN(pti, pdi, pdpti, pml4i);
                    IPTMapType maptype = getMapType(pt[pti]);
                    PageManager::instance()->decrementReferenceCount(pt[pti].page_ppn, vpn, this, maptype);
                    debug(FORK, "getReferenceCount in exec_destructor (decrement) %d \n", PageManager::instance()->getReferenceCount(pt[pti].page_ppn));
                    ((uint64*)pt)[pti] = 0;
                    PageManager::instance()->ref_count_lock_.release();
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
  archmemory_lock_.release();
  IPTManager::instance()->IPT_lock_.release();
}


int ArchMemory::isCOW(size_t virtual_addr)
{
  debug(A_MEMORY, "ArchMemory::isCow: with virtual address %p.\n", (void*)virtual_addr);
  assert(archmemory_lock_.heldBy() != currentThread);
  IPTManager::instance()->IPT_lock_.acquire();
  archmemory_lock_.acquire();

  ArchMemoryMapping m = ArchMemory::resolveMapping(virtual_addr/PAGE_SIZE);
  PageTableEntry* pml1_entry = &m.pt[m.pti];
  PageDirEntry* pml2_entry = &m.pd[m.pdi];

  if (pml2_entry && pml2_entry->pt.cow)
  {
    debug(COW, "ArchMemory::isCow: virtual address %p is COW with PML2\n", (void*)virtual_addr);
    return 2;
  }

  if (pml1_entry && pml1_entry->cow)
  {
    debug(COW, "ArchMemory::isCow: virtual address %p is cow with PML1\n", (void*)virtual_addr);
    return 1;
  }
  else
  {
    debug(A_MEMORY, "ArchMemory::isCow: virtual address %p is not cow\n", (void*)virtual_addr);
    IPTManager::instance()->IPT_lock_.release();
    archmemory_lock_.release();
    return 0;
  }
}


void ArchMemory::copyPage(size_t virtual_addr)
{
  assert(archmemory_lock_.heldBy() == currentThread);
  PageManager* pm = PageManager::instance();

  debug(FORK, "ArchMemory::copyPage Resolving mapping \n");
  ArchMemoryMapping pml1 = ArchMemory::resolveMapping(virtual_addr/PAGE_SIZE);
  PageTableEntry* pml1_entry = &pml1.pt[pml1.pti];
  assert(pml1_entry && pml1_entry->cow && "Page is not COW");

  if(pml1_entry->writeable == 1)
  {
    return;
  }

  debug(FORK, "ArchMemory::copyPage If the process is not the last process to own the page, then copy to new page\n");
  pm->ref_count_lock_.acquire();
  assert(pm->getReferenceCount(pml1_entry->page_ppn) > 0 && "Reference count is 0");
  if (pm->getReferenceCount(pml1_entry->page_ppn) > 1)
  {
    debug(FORK, "ArchMemory::copyPage: Ref count is > 1, Copying to a new page\n");
    size_t new_page_ppn = pm->allocPPN();
    pointer original_page = ArchMemory::getIdentAddressOfPPN(pml1_entry->page_ppn);
    pointer new_page = ArchMemory::getIdentAddressOfPPN(new_page_ppn);
    memcpy((void*)new_page, (void*)original_page, PAGE_SIZE);

    debug(FORK, "ArchMemory::copyPage update the ref count for old page and new page\n");

    IPTMapType maptype_old = getMapType(*pml1_entry);
    pm->decrementReferenceCount(pml1_entry->page_ppn, virtual_addr/PAGE_SIZE, this, maptype_old);
    pml1_entry->page_ppn = new_page_ppn;

    IPTMapType maptype_new = getMapType(*pml1_entry);
    pm->incrementReferenceCount(pml1_entry->page_ppn, virtual_addr/PAGE_SIZE, this, maptype_new);
    
  }
  else if (pm->getReferenceCount(pml1_entry->page_ppn) != 1 || pml1_entry->cow == 0)
  {
    debug(FORK, "ArchMemory::copyPage: Error! Refcount is %d, write bit: %ld, cow bit: %ld\n", pm->getReferenceCount(pml1_entry->page_ppn), pml1_entry->writeable, pml1_entry->cow);
    assert(0 && "ArchMemory::copyPage: More processes own this page than expected, because this page is being copied even tho its no longer COW\n ");
  }
  pm->ref_count_lock_.release();

  debug(FORK, "ArchMemory::copyPage Setting up the bit of the new page (present, write, !cow)\n");
  pml1_entry->writeable = 1;
  // pml1_entry->cow = 0; //not nessessary i think
}
void ArchMemory::copyPageTable(size_t virtual_addr)
{
  debug(FORK, "ArchMemory::copyPage Resolving mapping \n");
  ArchMemoryMapping m = ArchMemory::resolveMapping(virtual_addr/PAGE_SIZE);
  PageDirEntry* pml2_entry = &m.pd[m.pdi];

  size_t new_page_ppn = PageManager::instance()->allocPPN();

  PageTableEntry* original_page = (PageTableEntry*) getIdentAddressOfPPN(pml2_entry->pt.page_ppn);
  PageTableEntry* new_page = (PageTableEntry*) getIdentAddressOfPPN(new_page_ppn);

  memcpy((void*)new_page, (void*)original_page, PAGE_SIZE);

  PageManager::instance()->ref_count_lock_.acquire();
  debug(COW, "----getReferenceCount in CopyPageTable %d \n", PageManager::instance()->getReferenceCount(pml2_entry->pt.page_ppn));
  PageManager::instance()->decrementEntryReferenceCount(pml2_entry->pt.page_ppn);
  PageManager::instance()->ref_count_lock_.release();

  //update the page directory entry to point to the new physical page
  pml2_entry->pt.page_ppn = new_page_ppn;

  PageManager::instance()->ref_count_lock_.acquire();
  PageManager::instance()->incrementEntryReferenceCount(pml2_entry->pt.page_ppn);
  PageManager::instance()->ref_count_lock_.release();
  pml2_entry->pt.writeable = 1;
  pml2_entry->pt.cow = 0;

  for (uint64 pti = 0; pti < PAGE_TABLE_ENTRIES; pti++)
  {
    if (original_page[pti].present)
    {
      original_page[pti].writeable = 0; //read only
      original_page[pti].cow = 1;

      new_page[pti].writeable = 0; //read only
      new_page[pti].cow = 1;

      size_t vpn = construct_VPN(pti, m.pdi, m.pdpti, m.pml4i);
      PageManager::instance()->ref_count_lock_.acquire();
      PageManager::instance()->incrementReferenceCount(original_page[pti].page_ppn, vpn, this, IPTMapType::RAM_MAP);
      PageManager::instance()->ref_count_lock_.release();

    }
  }

}

bool ArchMemory::updatePageTableEntryForSwapOut(size_t vpn, size_t disk_offset)
{
  assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread);
  assert(archmemory_lock_.heldBy() == currentThread);

  ArchMemoryMapping mapping = resolveMapping(vpn);
  
  PageTableEntry* pt_entry = &mapping.pt[mapping.pti];
  assert(pt_entry && "No pagetable entry");

  pt_entry->present = 0;
  pt_entry->swapped_out = 1;

  pt_entry->page_ppn = disk_offset;

  return true;
}

size_t ArchMemory::getDiskLocation(size_t vpn)
{
  assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread);
  assert(archmemory_lock_.heldBy() == currentThread);

  ArchMemoryMapping mapping = resolveMapping(vpn);
  
  PageTableEntry* pt_entry = &mapping.pt[mapping.pti];
  assert(pt_entry && "No pagetable entry");

  if(!pt_entry->swapped_out || pt_entry->present)
  {
    assert(0 && "Page needs to be swapped out and not present");
  }
  else
  {
    return pt_entry->page_ppn; //ppn is used to store disk location
  }

  return true;
}


bool ArchMemory::isSwapped(size_t virtual_addr)
{
  debug(A_MEMORY, "ArchMemory::isSwapped: with virtual address %p.\n", (void*)virtual_addr);
  assert(archmemory_lock_.heldBy() == currentThread);
  ArchMemoryMapping m = ArchMemory::resolveMapping(virtual_addr/PAGE_SIZE);
  PageTableEntry* pt_entry = &m.pt[m.pti];

  if (m.pt && pt_entry && pt_entry->swapped_out)
  {
    debug(A_MEMORY, "ArchMemory::isSwapped: virtual address %p is swapped out\n", (void*)virtual_addr);
    return true;
  }
  else
  {
    debug(A_MEMORY, "ArchMemory::isSwapped: virtual address %p is not swapped out\n", (void*)virtual_addr);
    return false;
  }
}

bool ArchMemory::updatePageTableEntryForSwapIn(size_t vpn, size_t ppn)
{
  assert(IPTManager::instance()->IPT_lock_.heldBy() == currentThread);
  assert(archmemory_lock_.heldBy() == currentThread);

  ArchMemoryMapping mapping = resolveMapping(vpn);
  
  PageTableEntry* pt_entry = &mapping.pt[mapping.pti];
  assert(pt_entry && "No pagetable entry");

  pt_entry->present = 1;
  pt_entry->swapped_out = 0;

  pt_entry->page_ppn = ppn;

  return true;
}


size_t ArchMemory::construct_VPN(size_t pti, size_t pdi, size_t pdpti, size_t pml4i)
{
  VirtualAddress virtual_address;
  virtual_address.offset = 0;
  virtual_address.pti = pti;
  virtual_address.pdi = pdi;
  virtual_address.pdpti = pdpti;
  virtual_address.pml4i = pml4i;
  virtual_address.ignored = 0;

  return virtual_address.packed/PAGE_SIZE;
}


IPTMapType ArchMemory::getMapType(PageTableEntry& pt_entry)
{
  debug(A_MEMORY, "ArchMemory::getMapType called with %p\n.", &pt_entry);
  if(pt_entry.swapped_out)
  {
    debug(A_MEMORY, "ArchMemory::getMapType called returns DISK_MAP.\n");
    return IPTMapType::DISK_MAP;
  }
  else
  {
    debug(A_MEMORY, "ArchMemory::getMapType called returns RAM_MAP.\n");
    return IPTMapType::RAM_MAP;
  }
}
