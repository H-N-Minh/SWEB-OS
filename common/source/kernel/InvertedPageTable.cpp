#include "InvertedPageTable.h" 
#include "ArchMemory.h"
#include "Mutex.h"
#include "assert.h"
#include "UserProcess.h"
#include "UserThread.h"


InvertedPageTable* InvertedPageTable::instance_ = nullptr;

InvertedPageTable::InvertedPageTable():IPT_lock_("IPT_lock_")
{
  assert(!instance_);
  instance_ = this;
}

InvertedPageTable* InvertedPageTable::instance()
{
  return instance_;
}

InvertedPageTable::~InvertedPageTable()
{
  //delete values in ipt_disk
  for (auto& map_entry : ipt_disk_) 
  {
    for(auto& page_info : map_entry.second)
    {
      delete page_info;
    }
    map_entry.second.clear();
  }
  ipt_disk_.clear();

  //delete values in ipt_ram
  for (auto& map_entry : ipt_ram_) 
  {
    for(auto& page_info : map_entry.second)
    {
      delete page_info;
    }
    map_entry.second.clear();
  }
  ipt_ram_.clear();
}



 ustl::map<size_t, ustl::vector<VirtualPageInfo*>>& InvertedPageTable::selectMap(MAPTYPE map_type)
{
  if(map_type == IPT_RAM)
  {
    return ipt_ram_;
  }
  else if(map_type == IPT_DISK)
  {
    return ipt_disk_;
  }
  else
  {
    assert(0);
  }
}



bool InvertedPageTable::KeyisInMap(size_t key, MAPTYPE map_type) //key ppn or disk_offset
{
  debug(IPT, "InvertedPageTable::KeyisInMap: Check if key %ld is in map %s.\n", key, (map_type == MAPTYPE::IPT_RAM)?"IPT_RAM":"IPT_DISK");
  assert(IPT_lock_.heldBy() == currentThread);

  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>& ipt_map = selectMap(map_type);

  ustl::map<uint64, ustl::vector<VirtualPageInfo*>>::iterator iterator = ipt_map.find(key);                                                   
  if(iterator != ipt_map.end())
  {
    debug(IPT, "InvertedPageTable::KeyisInMap: At offset %ld is a value in the map %s.\n", key, (map_type == MAPTYPE::IPT_RAM)?"IPT_RAM":"IPT_DISK");
    return true;
  }
  else
  {
    return false;
  }
}


void InvertedPageTable::addVirtualPageInfo(size_t offset, size_t vpn, ArchMemory* archmemory, MAPTYPE map_type)
{ 
  debug(IPT, "InvertedPageTable::addVirtualPageInfo: Add at offset %ld in map %s the vpn %p and archmemory %p.\n", offset, (map_type == MAPTYPE::IPT_RAM)?"IPT_RAM":"IPT_DISK", (void*)vpn, archmemory);
  assert(IPT_lock_.heldBy() == currentThread);
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>& ipt_map = selectMap(map_type);

  VirtualPageInfo* new_info = new VirtualPageInfo{vpn, archmemory};
  ipt_map[offset].push_back(new_info);
}


void InvertedPageTable::removeVirtualPageInfo(size_t offset, size_t vpn, ArchMemory* archmemory, MAPTYPE map_type)
{
  debug(IPT, "InvertedPageTable::removeVirtualPageInfo: Remove at offset %ld in map %s the vpn %p and archmemory %p.\n", offset, (map_type == MAPTYPE::IPT_RAM)?"IPT_RAM":"IPT_DISK", (void*)vpn, archmemory);
  assert(IPT_lock_.heldBy() == currentThread);
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>& ipt_map = selectMap(map_type);
  assert(KeyisInMap(offset, map_type) && "No value at given offset in this map.\n");

  bool found = false;

  for(long unsigned int i = 0; i < ipt_map[offset].size(); i++)
  {
    if(ipt_map[offset][i]->vpn_== vpn && ipt_map[offset][i]->arch_memory_ ==  archmemory)
    {
      found = true;
      delete ipt_map[offset][i];
      ipt_map[offset].erase(ipt_map[offset].begin() + i);
      break;
    }
  }



  assert(found && "PageInfo not found in the map!");
}


ustl::vector<VirtualPageInfo*> InvertedPageTable::moveToOtherMap(size_t old_key, size_t new_key, MAPTYPE from, MAPTYPE to)
{
  debug(IPT, "InvertedPageTable::moveToOtherMap: Move infos at offset %ld in map %s to offset %ld in map %s.\n", old_key, (from == MAPTYPE::IPT_RAM)?"IPT_RAM":"IPT_DISK", new_key, (to == MAPTYPE::IPT_RAM)?"IPT_RAM":"IPT_DISK");
  assert(IPT_lock_.heldBy() == currentThread);

  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>& source_ipt_map = selectMap(from);
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>& destination_ipt_map = selectMap(to);

  assert(KeyisInMap(old_key, from) && "Key is not in old map");
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>::iterator iterator = source_ipt_map.find(old_key);  
  ustl::vector<VirtualPageInfo*> virtual_page_infos;                                                 
  if(iterator != source_ipt_map.end())
  {
    virtual_page_infos = source_ipt_map[old_key];
    source_ipt_map.erase(iterator);
  }
  else
  {
    assert(0 && "Key not found in the map!");
  }
  assert(!KeyisInMap(old_key, from) && "Key was not removed from old map");
  assert(!KeyisInMap(new_key, to) && "Key is already in new map");
  destination_ipt_map[new_key] = virtual_page_infos;
  assert(KeyisInMap(new_key, to) && "Key not successfully added to new map");

  return virtual_page_infos;
}




ustl::vector<VirtualPageInfo*> InvertedPageTable::getPageInfosForPPN(size_t ppn)
{
  assert(IPT_lock_.heldBy() == currentThread);
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

int InvertedPageTable::getNumPagesInMap(MAPTYPE maptype)
{
  ustl::map<size_t, ustl::vector<VirtualPageInfo*>>& ipt_map = selectMap(maptype);
  return ipt_map.size();
}

