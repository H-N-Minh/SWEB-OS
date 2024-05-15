#include "InvertedPageTable.h" 
#include "Mutex.h"
#include "ArchMemory.h"
#include "assert.h"
#include "UserProcess.h"
#include "UserThread.h"


bool InvertedPageTable::PPNisInMap(size_t ppn)
{
  assert(inverted_pagetable_lock_.heldBy() == currentThread);
  ustl::map<uint64, ustl::vector<VirtualPageInfo*>>::iterator iterator = inverted_pagetable_.find(ppn);                                                   
  if(iterator != inverted_pagetable_.end())
  {
    return true;
  }
  else
  {
    return false;
  }

}

InvertedPageTable::InvertedPageTable():inverted_pagetable_lock_("inverted_pagetable_lock_"){}

void InvertedPageTable::addPage(uint64 ppn, uint64 vpn, ArchMemory* archmemory)
{  
  VirtualPageInfo* new_info = new VirtualPageInfo{vpn, archmemory};
  inverted_pagetable_lock_.acquire();
  inverted_pagetable_[ppn].push_back(new_info);
  inverted_pagetable_lock_.release();
}


