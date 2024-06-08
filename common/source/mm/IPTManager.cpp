#include "IPTManager.h"
#include "debug.h"
#include "assert.h"
#include "debug.h"
#include "Thread.h"
#include "Syscall.h"
#include "uset.h"
#include "SwappingManager.h"
#include "ArchMemory.h"
#include "SwappingThread.h"


#define INVALID_PPN -1
#define UINT32_MAX 0xFFFFFFFF

class ArchmemIPT;

////////////////////// IPTManager //////////////////////
IPTManager* IPTManager::instance_ = nullptr;

IPTManager::IPTManager() 
  : IPT_lock_("IPTManager::IPT_lock_")
{
  assert(!instance_);
  instance_ = this;
  pra_type_ = PRA_TYPE::RANDOM;
}

IPTManager::~IPTManager()
{
  //delete values in ipt_disk
  for (auto& map_entry : ram_map_) 
  {
    delete map_entry.second;
  }
  ram_map_.clear();

  //delete values in ipt_ram
  for (auto& map_entry : disk_map_) 
  {
    delete map_entry.second;
  }
  disk_map_.clear();
}

IPTManager* IPTManager::instance()
{
  return instance_;
}

size_t randomNumGenerator()
{
  size_t time_stamp = (size_t) Syscall::get_current_timestamp_64_bit();
  return time_stamp / 73;
}

// only for debugging, used to see if the random number generator is working
void IPTManager::debugRandomGenerator()
{
  ustl::map<int, int> myMap;

  for (size_t i = 0; i < 2000000; i++)
  {
    size_t randomNum = randomNumGenerator();
    int randomNum_resized = randomNum % 1024;
    if (myMap.find(randomNum_resized) != myMap.end())
    {
      myMap[randomNum_resized]++;
    }
    else
    {
      myMap[randomNum_resized] = 1;
    }
  }
  
  ustl::vector<ustl::pair<int, int>> numberPairs;
  for (const auto& pair : myMap) {
    numberPairs.push_back(pair);
  }
  struct Compare {
    bool operator()(const ustl::pair<int, int>& a, const ustl::pair<int, int>& b) {
      return a.second < b.second;
    }
  };
  ustl::sort(numberPairs.begin(), numberPairs.end(), Compare());

  debug(MINH, "\n\nsort by number of time showing up\n");
  for (const auto& pair : numberPairs)
  {
    debug(MINH, "number: %d, count: %d\n", pair.first, pair.second);
  }

  struct Compare2 {
    bool operator()(const ustl::pair<int, int>& a, const ustl::pair<int, int>& b) {
      return a.first < b.first;
    }
  };
  ustl::sort(numberPairs.begin(), numberPairs.end(), Compare2());

  debug(MINH, "\n\nsort by value of generated number\n");
  for (const auto& pair : numberPairs)
  {
    debug(MINH, "number: %d, count: %d\n", pair.first, pair.second);
  }
}

size_t IPTManager::findPageToSwapOut()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::findPageToSwapOut called but IPT not locked\n");
  
  int ppn_retval = INVALID_PPN;

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();

  if (pra_type_ == PRA_TYPE::RANDOM)
  {
    debug(IPT, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA RANDOM\n");

    size_t random_num = randomNumGenerator();
    // debug(MINH, "IPTManager::findPageToSwapOut: random num : %zu\n", random_num);
    
    ustl::vector<ppn_t> unique_keys;
    for (const auto& pair : ram_map_)
    {
      unique_keys.push_back(pair.first);
    }
    if(unique_keys.size() == 0)
    {
      assert(0 && "No page in ram");
    }
    size_t random_ipt_index = random_num % unique_keys.size();
    ppn_retval = (size_t) (unique_keys[random_ipt_index]);
    debug(IPT, "IPTManager::findPageToSwapOut: Found random page to swap out: ppn=%d\n", ppn_retval);
  }
  else if (pra_type_ == PRA_TYPE::NFU)
  {
    debug(IPT, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA NFU\n");
    uint32 min_counter = UINT32_MAX;
    ustl::vector<uint32> min_ppns;    // vector of all pages with the minimum counter

    // go through ram_map_ and find the page with the lowest access counter
    assert(ram_map_.size() > 0 && "IPTManager::findPageToSwapOut: ram_map_ is empty. this should never happen\n");
    for(auto& pair : ram_map_)
    {
      ppn_t key = pair.first;
      uint32 counter = pair.second->access_counter_;
      if (counter < min_counter)
      {
        min_counter = counter;
        min_ppns.clear();
        ppn_retval = key;
        min_ppns.push_back(key);
      }
      else if (counter == min_counter)
      {
        min_ppns.push_back(key);
      }
    }
    debug(IPT, "IPTManager::findPageToSwapOut: Found %zu pages with the minimum counter: %d\n", min_ppns.size(), min_counter);
    if (min_ppns.size() > 1)
    {
      debug(IPT, "IPTManager::findPageToSwapOut: Multiple pages with the minimum counter. Randomly selecting one\n");
      size_t random_num = randomNumGenerator();
      size_t random_index = random_num % min_ppns.size();
      ppn_retval = min_ppns[random_index];
    }

    debug(SWAPPING, "IPTManager::findPageToSwapOut: Found page to swap out: ppn=%d, counter=%d\n", ppn_retval, min_counter);
  }
  else if(pra_type_ == PRA_TYPE::SIMPLE)
  {
    int counter = 0;
    bool key_in_ipt = false;
    while(!key_in_ipt)
    {
      counter++;
      if(counter == 2000)
      {
        assert(0 && "No page to swap out\n");
      }
      ppn_retval++;    
      if(ppn_retval > 2016)
      {
        ppn_retval = 1009;
      }
      key_in_ipt = isKeyInMap(ppn_retval, IPTMapType::RAM_MAP);
    }
  }
  else if(pra_type_ == PRA_TYPE::SECOND_CHANGE)
  {
    if(fifo_ppns.empty())
    {
      assert(0);
    }
    int counter = 0;
    while(ppn_retval == INVALID_PPN && counter < 2)
    {
      counter++;
      for(auto& ppn : fifo_ppns)
      {
        IPTEntry* entry = ram_map_[ppn];
        bool not_accessed = true;
        for(auto& archmem_ipt : entry->getArchmemIPTs())
        {
          ArchMemory* archmem = archmem_ipt->archmem_;
          size_t vpn = archmem_ipt->vpn_;
          if(archmem->isPageAccessed(vpn))
          {
            not_accessed = false;
            archmem->resetAccessBits(vpn);
          }
        }
        if(not_accessed)
        {
          ppn_retval = ppn;
          break;
        }
      }
    }
  }
  assert(ppn_retval != INVALID_PPN && "IPTManager::findPageToSwapOut: failed to find a valid ppn\n");
  return ppn_retval;
}

void IPTManager::insertEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  debug(SWAPPING, "IPTManager::insertEntryIPT: inserting ppn: %zu, vpn: %zu, archmem: %p to %s\n", ppn, vpn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::insertEntryIPT called without fully locking\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  
  // Error checking: entry should not already exist
  if (isEntryInMap(ppn, map_type, archmem, vpn))
  {
    debug(IPT, "IPTManager::insertEntryIPT: Entry (ppn: %zu, archmem: %p) already exists in %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::insertEntryIPT: Entry already exists in map\n");
  }

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();
  
  debug(IPT, "IPTManager::insertEntryIPT: Entry does not exist in %s yet, seems valid. Inserting\n", (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  if(isKeyInMap(ppn, map_type))
  {
    IPTEntry* entry = (*map)[ppn];
    assert(entry && "IPTManager::insertIPT: entry is null");

    entry->addArchmemIPT(vpn, archmem);
  }
  else
  {
    (*map)[ppn] = new IPTEntry();
    IPTEntry* entry = (*map)[ppn];
    entry->addArchmemIPT(vpn, archmem);

    fifo_ppns.push_back(ppn);
  }

  debug(IPT, "IPTManager::insertIPT: successfully inserted to IPT\n");

}

void IPTManager::removeEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  debug(IPT, "IPTManager::removeEntryIPT: removing ppn: %zx, archmem: %p from map %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::removeEntryIPT called but not fully locked\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);

  // Error checking that the entry does exist in the map before removing
  if (!isEntryInMap(ppn, map_type, archmem, vpn))
  {
    debug(IPT, "IPTManager::removeEntryIPT Entry (ppn %zu, archmem %p) not found in the map %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::removeEntryIPT: ppn doesnt exist in map\n");
  }
  
  // remove the item and update debug info
  debug(IPT, "IPTManager::removeEntryIPT: Entry found in map %s, seems valid. Removing\n", (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  IPTEntry* entry = (*map)[ppn];
  entry->removeArchmemIPT(vpn, archmem);
  if (entry->isEmpty())
  {
    delete entry;
    map->erase(ppn);

    auto it = ustl::find(fifo_ppns.begin(), fifo_ppns.end(), ppn);
    if (it != fifo_ppns.end())
    {
      fifo_ppns.erase(it);
    }
    else
    {
      assert(0 && "PPN not found in fifo_ppns vector");
    }
  }

  debug(IPT, "IPTManager::removeIPT: successfully removed from IPT\n");

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();
}

void IPTManager::moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::moveEntry called but IPT not locked\n");
  // TODO: assert that all archmem of the entry are locked

  auto* source_map                  = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  auto* destination_map             = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);
  const char* source_as_string      = (source == IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");
  const char* destination_as_string = (source == IPTMapType::RAM_MAP ? "DISK-MAP" : "RAM-MAP");
  IPTMapType destination_map_type   = (source == IPTMapType::RAM_MAP ? IPTMapType::DISK_MAP : IPTMapType::RAM_MAP);

  debug(SWAPPING, "IPTManager::moveEntry: moving entry at offset %zu in %s to offset %zu in %s\n", ppn_source, source_as_string, ppn_destination, destination_as_string);
  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();


  // Check if the move is valid (entry to be moved exists in source map, and does not exist in destination map)
  // also check if the archmem are locked
  if (!isKeyInMap(ppn_source, source))
  {
    debug(IPT, "IPTManager::moveEntry: Entry to be moved (ppn %zu) not found in source map %s\n", ppn_source, source_as_string);
    assert(0 && "IPTManager::moveEntry: Entry to be moved not found in source map\n");
  }
  
  IPTEntry* entry = (*source_map)[ppn_source];
  assert(entry && "IPTManager::moveEntry: entry is null");
  ustl::vector<ArchmemIPT*> archmemIPTs_vector = entry->getArchmemIPTs();
  assert(archmemIPTs_vector.size() > 0 && "IPTManager::moveEntry: archmemIPTs_vector is empty even tho the IPTentry exists\n");

  for (auto entry : archmemIPTs_vector)
  {
    assert(entry->isLockedByUs() && "IPTManager::moveEntry: ArchMemory not locked while moving entry\n");
    assert(!isEntryInMap(ppn_destination, destination_map_type, entry->archmem_,  entry->vpn_) && "IPTManager::moveEntry: Entry to be moved already exists in destination map\n");
  }
  debug(SWAPPING, "IPTManager::moveEntry: Entry to be moved seems valid, moving now\n");

  // Moving entries
  (*destination_map)[ppn_destination] = entry;
  source_map->erase(ppn_source);
  entry->access_counter_ = 0;

  if(ppn_destination == IPTMapType::RAM_MAP)
  {
    fifo_ppns.push_back(ppn_destination);
  }
}

void IPTManager::removeEntry(IPTMapType map_type, size_t ppn)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::removeEntry called but IPT not locked\n");

  if (!isKeyInMap(ppn, map_type))
  {
    assert(0 && "IPTManager::removeEntry: PPN not found in map\n");
  }
  auto& map = (map_type == IPTMapType::RAM_MAP ? ram_map_ : disk_map_);

  map.erase(ppn);

  if (isKeyInMap(ppn, map_type))
  {
    assert(0 && "IPTManager::removeEntry: PPN after removing still in map\n");
  }

}


bool IPTManager::isEntryInMap(size_t ppn, IPTMapType maptype, ArchMemory* archmem, size_t vpn)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::isEntryInMap called but IPT not locked\n");
  auto* map = (maptype == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);

  auto it = map->find(ppn);
  if (it == map->end())
  {
    return false;
  }
  else
  {
    return it->second->isArchmemExist(archmem, vpn);            //TODO: it->second->vpn_ == vpn VPN!!!!!!!!!!!!!
  }
}

bool IPTManager::isKeyInMap(size_t offset, IPTMapType maptype)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::isKeyInMap called but IPT not locked\n");
  auto* map = (maptype == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);

  if (map->find(offset) == map->end())
  {
    return false;
  }
  else
  {
    return true;
  }
}

int IPTManager::getNumPagesInMap(IPTMapType maptype)
{
  if(maptype == IPTMapType::RAM_MAP)
  {
    return ram_map_.size();
  }
  else
  {
    return disk_map_.size();
  }
}

void IPTManager::checkRamMapConsistency()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::checkRamMapConsistency called but IPT not locked\n");
  assert(ram_map_.size() && "IPTManager::checkRamMapConsistency: ram_map_ is empty, unlikely to happen\n");

  for (auto it = ram_map_.begin(); it != ram_map_.end(); ++it)
  {
    ppn_t key = it->first;
    IPTEntry* ipt_entry = it->second;
    assert(ipt_entry && "checkRampMapConsistency: No IPTEntry");
    ustl::vector<ArchmemIPT*> archmemIPTs_vector = ipt_entry->getArchmemIPTs();
    assert(archmemIPTs_vector.size() && "checkRampMapConsistency: No ArchmemIPT (empty archmem vector), but IPTEntry still exists in ram_map_");

    for (auto archmemIPT : archmemIPTs_vector)
    {
      ArchMemory* entry_arch = archmemIPT->archmem_;
      size_t vpn = archmemIPT->vpn_;
      assert(entry_arch && "No archmem in ArchmemIPT");

      // this locking will not solve deadlock completely, but this is debug func so who cares
      int locked_by_us = 0;
      if (!entry_arch->archmemory_lock_.isHeldBy((Thread*) currentThread))
      {
        entry_arch->archmemory_lock_.acquire();
        locked_by_us = 1;
      }

      ArchMemoryMapping mapping = entry_arch->resolveMapping(vpn);
      PageTableEntry* pt_entry = &mapping.pt[mapping.pti];
      assert(pt_entry && "checkRampMapConsistency: No pagetable entry");

      if(!pt_entry->present)
      {
        assert(0 && "checkRampMapConsistency: page is not present, but exists in ram_map_\n");
      }
      else
      {
        assert(pt_entry->page_ppn == key && "checkRampMapConsistency: ppn in ram_map_ (key) does not match ppn in ArchMemory\n");
      }

      if (locked_by_us)
      {
        entry_arch->archmemory_lock_.release();
      }

    }
  }
}

void IPTManager::checkDiskMapConsistency()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::checkDiskMapConsistency called but IPT not locked\n");

  for (auto it = disk_map_.begin(); it != disk_map_.end(); ++it)
  {
    ppn_t key = it->first;
    IPTEntry* ipt_entry = it->second;
    assert(ipt_entry && "checkRampMapConsistency: No IPTEntry");
    ustl::vector<ArchmemIPT*> archmemIPTs_vector = ipt_entry->getArchmemIPTs();
    assert(archmemIPTs_vector.size() && "checkRampMapConsistency: No ArchmemIPT (empty archmem vector), but IPTEntry still exist in diskmap");

    for (auto archmemIPT : archmemIPTs_vector)
    {
      ArchMemory* entry_arch = archmemIPT->archmem_;
      size_t vpn = archmemIPT->vpn_;
      assert(entry_arch && "No archmem in ArchmemIPT");

      // this locking will not solve deadlock completely, but this is debug func so who cares
      int locked_by_us = 0;
      if (!entry_arch->archmemory_lock_.isHeldBy((Thread*) currentThread))
      {
        entry_arch->archmemory_lock_.acquire();
        locked_by_us = 1;
      }
      
      ppn_t disk_offset = (ppn_t) entry_arch->getDiskLocation(vpn);
      assert(disk_offset && "checkRampMapConsistency: disk_offset is 0\n");
      assert(key == disk_offset && "checkRampMapConsistency: ppn in disk_map_ (key) does not match disk offset in ArchMemory\n");
      if (locked_by_us)
      {
        entry_arch->archmemory_lock_.release();
      }
    }
  }
}
