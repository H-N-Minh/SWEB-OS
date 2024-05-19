#include "InvertedPageTable.h" 
#include "ArchMemory.h"
#include "Mutex.h"
#include "assert.h"
#include "UserProcess.h"
#include "UserThread.h"

 ustl::map<size_t, ustl::vector<VirtualPageInfo*>>* InvertedPageTable::selectMap(MAPTYPE map_type)
{
  if(map_type == IPT_RAM)
  {
    return &ipt_ram_;
  }
  else if(map_type == IPT_DISK)
  {
    return &ipt_disk_;
  }
  else
  {
    assert(0);
  }
}


InvertedPageTable* InvertedPageTable::instance_ = nullptr;

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


bool InvertedPageTable::KeyisInMap(size_t key, MAPTYPE map_type) //key ppn or disk_offset
{
  assert(ipt_lock_.heldBy() == currentThread);

  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>* ipt_map = selectMap(map_type);

  ustl::map<uint64, ustl::vector<VirtualPageInfo*>>::iterator iterator = ipt_map->find(key);                                                   
  if(iterator != ipt_map->end())
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
  ipt_ram_[ppn].push_back(new_info);  //TODOs: test
}


ustl::vector<VirtualPageInfo*> InvertedPageTable::moveToOtherMap(size_t old_key, size_t new_key, MAPTYPE from, MAPTYPE to)
{
  assert(ipt_lock_.heldBy() == currentThread);

  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>* source_ipt_map = selectMap(from);
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>* destination_ipt_map = selectMap(to);

  assert(KeyisInMap(old_key, from) && "Key is not in old map");
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>::iterator iterator = source_ipt_map->find(old_key);  
  ustl::vector<VirtualPageInfo*> virtual_page_infos;                                                 
  if(iterator != source_ipt_map->end())
  {
    virtual_page_infos = (*source_ipt_map)[old_key];
    source_ipt_map->erase(iterator);
    // return virtual_page_infos;
  }
  else
  {
    assert(0 && "Key not found in the map!");
  }
  assert(!KeyisInMap(old_key, from) && "Key was not removed from old map");
  assert(!KeyisInMap(new_key, to) && "Key is already in new map");
  (*destination_ipt_map)[new_key] = virtual_page_infos;
  assert(KeyisInMap(new_key, to) && "Key not successfully added to new map");

  return virtual_page_infos;
}






//---------DEBUG_METHODS------------------------------------------------------------------------------------
ustl::vector<VirtualPageInfo*> InvertedPageTable::getPageInfosForPPN(size_t ppn)
{
  assert(ipt_lock_.heldBy() == currentThread);
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>::iterator iterator = ipt_ram_.find(ppn);                                                   
  if(iterator != ipt_ram_.end())
  {
    ustl::vector<VirtualPageInfo*> virtual_page_infos = ipt_ram_[ppn];
    return virtual_page_infos;
  }
  else
  {
    ustl::vector<VirtualPageInfo*> empty;
    return empty;
  }
}

