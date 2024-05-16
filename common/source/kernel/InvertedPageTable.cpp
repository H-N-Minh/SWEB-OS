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


void InvertedPageTable::addVirtualPageInfo(size_t ppn, size_t vpn, ArchMemory* archmemory)
{  
  assert(ipt_lock_.heldBy() == currentThread);
  VirtualPageInfo* new_info = new VirtualPageInfo{vpn, archmemory};
  ipt_[ppn].push_back(new_info);  //TODOs: test
}

ustl::vector<VirtualPageInfo*> InvertedPageTable::getAndRemoveVirtualPageInfos(size_t ppn)
{
  assert(ipt_lock_.heldBy() == currentThread);
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>::iterator iterator = ipt_.find(ppn);                                                   
  if(iterator != ipt_.end())
  {
    ustl::vector<VirtualPageInfo*> virtual_page_infos = ipt_[ppn];
    ipt_.erase(iterator);
    return virtual_page_infos;
  }
  else
  {
    assert(0 && "PPN not found in the map!");
  }
}

void InvertedPageTable::addVirtualPageInfos(size_t ppn, ustl::vector<VirtualPageInfo*> page_infos)
{
  assert(ipt_lock_.heldBy() == currentThread);
  assert(!PPNisInMap(ppn) && "page is already in map");
  ipt_[ppn] = page_infos;
}

