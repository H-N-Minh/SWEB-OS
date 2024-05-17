#include "InvertedPageTable2.h" 
#include "ArchMemory.h"
#include "Mutex.h"
#include "assert.h"
#include "UserProcess.h"
#include "UserThread.h"

InvertedPageTable2* InvertedPageTable2::instance_ = nullptr;

InvertedPageTable2::InvertedPageTable2():ipt2_lock_("ipt2_lock_")
{
  assert(!instance_);
  instance_ = this;
}

InvertedPageTable2* InvertedPageTable2::instance()
{
  assert(instance_);
  return instance_;
}


bool InvertedPageTable2::PPNisInMap(size_t ppn)
{
  assert(ipt2_lock_.heldBy() == currentThread);
  ustl::map<uint64, ustl::vector<VirtualPageInfo*>>::iterator iterator = ipt2_.find(ppn);                                                   
  if(iterator != ipt2_.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}


void InvertedPageTable2::addVirtualPageInfo(size_t ppn, size_t vpn, ArchMemory* archmemory)
{  
  assert(ipt2_lock_.heldBy() == currentThread);
  VirtualPageInfo* new_info = new VirtualPageInfo{vpn, archmemory};
  ipt2_[ppn].push_back(new_info);
}

ustl::vector<VirtualPageInfo*> InvertedPageTable2::getAndRemoveVirtualPageInfos(size_t ppn)
{
  assert(ipt2_lock_.heldBy() == currentThread);
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>::iterator iterator = ipt2_.find(ppn);                                                   
  if(iterator != ipt2_.end())
  {
    ustl::vector<VirtualPageInfo*> virtual_page_infos = ipt2_[ppn];
    ipt2_.erase(iterator);
    return virtual_page_infos;
  }
  else
  {
    assert(0 && "PPN not found in the map!");
  }
}

void InvertedPageTable2::addVirtualPageInfos(size_t ppn, ustl::vector<VirtualPageInfo*> page_infos)
{
  assert(ipt2_lock_.heldBy() == currentThread);
  assert(!PPNisInMap(ppn) && "page is already in map");
  ipt2_[ppn] = page_infos;
}

