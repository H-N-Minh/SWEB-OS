#include "InvertedPageTable.h" 
#include "ArchMemory.h"
#include "Mutex.h"
#include "assert.h"
#include "UserProcess.h"
#include "UserThread.h"


InvertedPageTable::InvertedPageTable():ipt_lock_("ipt_lock_")
{
  assert(!instance_);
  instance_ = this;
}

InvertedPageTable* InvertedPageTable::instance()
{
  assert(instance_);
  return instance_;
}


bool InvertedPageTable::PPNisInMap(size_t ppn)
{
  assert(ipt_lock_.heldBy() == currentThread);
  ustl::map<uint64, ustl::vector<VirtualPageInfo*>>::iterator iterator = ipt_.find(ppn);                                                   
  if(iterator != ipt_.end())
  {
    return true;
  }
  else
  {
    return false;
  }

}


void InvertedPageTable::addPage(uint64 ppn, uint64 vpn, ArchMemory* archmemory)
{  
  assert(ipt_lock_.heldBy() == currentThread);
  VirtualPageInfo* new_info = new VirtualPageInfo{vpn, archmemory};
  ipt_[ppn].push_back(new_info);
}


