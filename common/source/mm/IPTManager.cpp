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

////////////////////// IPTEntry //////////////////////

IPTEntry::IPTEntry(size_t vpn, ArchMemory* archmem) : vpn_(vpn), archmem_(archmem) {}

bool IPTEntry::isLocked()
{
  return archmem_->archmemory_lock_.isHeldBy((Thread*) currentThread);
}


////////////////////// IPTManager //////////////////////
IPTManager* IPTManager::instance_ = nullptr;

IPTManager::IPTManager() 
  : IPT_lock_("IPTManager::IPT_lock_")
{
  assert(!instance_);
  instance_ = this;
  pra_type_ = PRA_TYPE::NFU;
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

ustl::vector<ppn_t> IPTManager::getUniqueKeysInRamMap()
{
  ustl::set<ppn_t> unique_keys;
  for (auto it = ram_map_.begin(); it != ram_map_.end(); ++it) {
    unique_keys.insert(it->first);
  }

  return ustl::vector<ppn_t>(unique_keys.begin(), unique_keys.end());
}


size_t IPTManager::findPageToSwapOut()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::findPageToSwapOut called but IPT not locked\n");
  
  int ppn_retval = INVALID_PPN;

  // checkSwapMetaDataConsistency();

  if (pra_type_ == PRA_TYPE::RANDOM)
  {
    debug(IPT, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA RANDOM\n");

    size_t random_num = randomNumGenerator();
    debug(MINH, "IPTManager::findPageToSwapOut: random num : %zu\n", random_num);
    
    ustl::vector<ppn_t> unique_keys = getUniqueKeysInRamMap();
    size_t random_ipt_index = random_num % unique_keys.size();
    
    ppn_retval = (size_t) (unique_keys[random_ipt_index]);
    debug(IPT, "IPTManager::findPageToSwapOut: Found random page to swap out: ppn=%d\n", ppn_retval);
  }
  else if (pra_type_ == PRA_TYPE::NFU)
  {
    debug(IPT, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA NFU\n");
    uint32 min_counter = UINT32_MAX;
    ustl::vector<uint32> min_ppns;    // vector of all pages with the minimum counter

    // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
    // checkRamMapConsistency();
    // checkDiskMapConsistency();
    // checkSwapMetaDataConsistency();
    

    assert(swap_meta_data_.size() > 0 && "IPTManager::findPageToSwapOut: swap_meta_data_ is empty. this should never happen\n");
    for(auto& pair : swap_meta_data_)
    {
      ppn_t key = pair.first;
      uint32 counter = pair.second;
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
      // DEBUGMINH (delete this)
      // for (auto ppn : min_ppns)
      // {
      //   debug(MINH, "IPTManager::findPageToSwapOut: candidate=%d, counter=%d\n", ppn, swap_meta_data_[ppn]);
      // }

      debug(IPT, "IPTManager::findPageToSwapOut: Multiple pages with the minimum counter. Randomly selecting one\n");
      size_t random_num = randomNumGenerator();
      size_t random_index = random_num % min_ppns.size();
      ppn_retval = min_ppns[random_index];
    }

    debug(SWAPPING, "IPTManager::findPageToSwapOut: Found page to swap out: ppn=%d, counter=%d\n", ppn_retval, min_counter);
  }

  assert(ppn_retval != INVALID_PPN && "IPTManager::findPageToSwapOut: failed to find a valid ppn\n");
  return ppn_retval;
}


IPTEntry* IPTManager::lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::lookupEntryInRAM: Looking up entry in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
  auto range = ram_map_.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it)
  {
    if (it->second->vpn_ == vpn && it->second->archmem_ == archmem)
    {
      debug(IPT, "IPTManager::lookupEntryInRAM: Entry found in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
      return it->second;
    }
  }
  debug(IPT, "IPTManager::lookupEntryInRAM: Entry not found in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
  return nullptr;
}

void IPTManager::insertEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  debug(SWAPPING, "IPTManager::insertEntryIPT: inserting ppn: %zu, vpn: %zu, archmem: %p to %s\n", ppn, vpn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::insertEntryIPT called without fully locking\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  
  // Error checking: entry should not already exist
  if (isEntryInMap(ppn, map_type, archmem))
  {
    debug(IPT, "IPTManager::insertEntryIPT: Entry (ppn: %zu, archmem: %p) already exists in %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::insertEntryIPT: Entry already exists in map\n");
  }

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();
  // checkSwapMetaDataConsistency();
  
  debug(IPT, "IPTManager::insertEntryIPT: Entry does not exist in %s yet, seems valid. Inserting\n", (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  // update debugging info, also update the swap_meta_data
  if(!isKeyInMap(ppn, map_type))  // before the insert, the key doesnt exist in map yet -> update the debug info
  {
    if(map_type == IPTMapType::RAM_MAP)
    {
      pages_in_ram_++;
      swap_meta_data_[ppn] = 0;
    }
    else
    {
      pages_on_disk_++;
    }
  }
  
  // Actually insert the pte to the entry
  IPTEntry* entry = new IPTEntry(vpn, archmem);
  map->insert({ppn, entry});

  debug(IPT, "IPTManager::insertIPT: successfully inserted to IPT\n");

}

void IPTManager::removeEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  debug(IPT, "IPTManager::removeEntryIPT: removing ppn: %zx, archmem: %p from map %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::removeEntryIPT called but not fully locked\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  
  // Error checking that the entry does exist in the map before removing
  if (!isEntryInMap(ppn, map_type, archmem))
  {
    debug(IPT, "IPTManager::removeEntryIPT Entry (ppn %zu, archmem %p) not found in the map %s\n", ppn, archmem, (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
    assert(0 && "IPTManager::removeEntryIPT: ppn doesnt exist in map\n");
  }
  
  
  // debug(IPT, "IPTManager::removeEntryIPT: Entry found in map %s, seems valid. Removing\n", (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));
  
  // Actually remove the pte from the entry
  auto range = map->equal_range(ppn);
  for (auto it = range.first; it != range.second;)
  {
    if (it->second->vpn_ == vpn && it->second->archmem_ == archmem)
    {
      // debug(IPT, "IPTManager::removeEntryFromRAM: Entry found and removed from RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
      IPTEntry* entry = it->second;
      it = map->erase(it);  // erase the value, the key still exists if theres other values mapped to it
      delete entry;    // delete the pointer created using "new"
      break;
    }
    else
    {
      ++it;
    }
  }

  // update debugging info, also update the swap_meta_data
  if(!isKeyInMap(ppn, map_type))  // after remove, the key does not exist in the map anymore -> update the debug info
  {
    if(map_type == IPTMapType::RAM_MAP)
    {
      pages_in_ram_--;
      swap_meta_data_.erase(ppn);
    }
    else
    {
      pages_on_disk_--;
    }
  }

  debug(IPT, "IPTManager::removeIPT: successfully removed from IPT\n");

  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();
  // checkSwapMetaDataConsistency();
}

ustl::vector<IPTEntry*> IPTManager::moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
  const char* source_as_string = (source == IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");
  const char* destination_as_string = (source != IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");

  debug(SWAPPING, "IPTManager::moveEntry: moving entry at offset %zu in %s to offset %zu in %s\n", ppn_source, source_as_string, ppn_destination, destination_as_string);
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::moveEntry called but IPT not locked\n");

  auto* source_map = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  auto* destination_map = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);


  // This is not necessary and slow down the system, can be commented out, but it is good for preventing error
  // checkRamMapConsistency();
  // checkDiskMapConsistency();
  // checkSwapMetaDataConsistency();


  // Check if the entry already exists in the destination map
  // also check if the archmem are locked
  // also check if the swap_meta_data is in sync with the ram_map_
  ustl::vector<IPTEntry*> ipt_entries;

  if (source == IPTMapType::RAM_MAP)
  {
    ipt_entries = getRamEntriesFromKey(ppn_source);
    if (swap_meta_data_.find(ppn_source) == swap_meta_data_.end())
    {
      debug(SWAPPING, "IPTManager::moveEntry: Entry %zu to be moved does not exist in the swap_meta_data\n", ppn_source);
      assert(0 && "IPTManager::moveEntry: Entry to be moved out does not exist in the swap_meta_data, likely because swap_meta_data is not in sync with ram_map_\n");
    }
  }
  else
  {
    ipt_entries = getDiskEntriesFromKey(ppn_source);
    if (swap_meta_data_.find(ppn_destination) != swap_meta_data_.end())
    {
      debug(SWAPPING, "IPTManager::moveEntry: Entry %zu to be moved already exists in the swap_meta_data\n", ppn_source);
      assert(0 && "IPTManager::moveEntry: Entry to be moved in already exists in the swap_meta_data, likely because swap_meta_data is not in sync with ram_map_\n");
    }
  }
  assert(ipt_entries.size() > 0 && "IPTManager::moveEntry: Entry to be moved is not found in source map\n");
  for (auto entry : ipt_entries)
  {
    assert(entry->archmem_->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::moveEntry: ArchMemory not locked while moving entry\n");
    IPTMapType destination_map_type = (source == IPTMapType::RAM_MAP ? IPTMapType::DISK_MAP : IPTMapType::RAM_MAP);
    assert(!isEntryInMap(ppn_destination, destination_map_type, entry->archmem_) && "IPTManager::moveEntry: Entry to be moved already exists in destination map\n");
  }
  debug(SWAPPING, "IPTManager::moveEntry: Entry to be moved seems valid, moving now\n");

  // Moving entries
  auto range = source_map->equal_range(ppn_source);
  for (auto it = range.first; it != range.second; ++it)
  {
    destination_map->insert({ppn_destination, it->second});
  }
  source_map->erase(ppn_source);

  // updating debuging info, also update the swap_meta_data
  if(source == IPTMapType::DISK_MAP)
  {
    swap_meta_data_[ppn_destination] = 0;
    pages_in_ram_++;
    pages_on_disk_--;
  }
  else
  {
    swap_meta_data_.erase(ppn_source);
    pages_on_disk_++;
    pages_in_ram_--;
  }

  return ipt_entries;
}

bool IPTManager::isEntryInMap(size_t ppn, IPTMapType maptype, ArchMemory* archmem)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::isEntryInMap called but IPT not locked\n");
  auto* map = (maptype == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  auto range = map->equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it)
  {
    if (it->second->archmem_ == archmem)
    {
      return true;
    }
  }
  return false;
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
    return pages_in_ram_;
  }
  else
  {
    return pages_on_disk_;
  }
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

ustl::vector<IPTEntry*> IPTManager::getRamEntriesFromKey(size_t ppn)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::getRamEntriesFromKey called but IPT not locked\n");
  ustl::vector<IPTEntry*> ram_entries;
  auto range = ram_map_.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it)
  {
    ram_entries.push_back(it->second);
  }
  return ram_entries;
}

ustl::vector<IPTEntry*> IPTManager::getDiskEntriesFromKey(size_t disk_offset)
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::getDiskEntriesFromKey called but IPT not locked\n");
  ustl::vector<IPTEntry*> disk_entries;
  auto range = disk_map_.equal_range(disk_offset);
  for (auto it = range.first; it != range.second; ++it)
  {
    disk_entries.push_back(it->second);
  }
  return disk_entries;
}

void IPTManager::checkRamMapConsistency()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::checkRamMapConsistency called but IPT not locked\n");
  for (auto it = ram_map_.begin(); it != ram_map_.end(); ++it)
  {
    ppn_t key = it->first;
    IPTEntry* ipt_entry = it->second;
    ArchMemory* entry_arch = ipt_entry->archmem_;
    assert(entry_arch && "No archmem in IPTEntry");

    // this locking will not solve deadlock completely, but this is debug func so who cares
    int locked_by_us = 0;
    if (!entry_arch->archmemory_lock_.isHeldBy((Thread*) currentThread))
    {
      entry_arch->archmemory_lock_.acquire();
      locked_by_us = 1;
    }

    ArchMemoryMapping mapping = entry_arch->resolveMapping((size_t) ipt_entry->vpn_);
    PageTableEntry* pt_entry = &mapping.pt[mapping.pti];
    assert(pt_entry && "checkRampMapConsistency: No pagetable entry");

    if(!pt_entry->present)
    {
      assert(0 && "checkRampMapConsistency: page is not present");
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

void IPTManager::checkDiskMapConsistency()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::checkDiskMapConsistency called but IPT not locked\n");

  for (auto it = disk_map_.begin(); it != disk_map_.end(); ++it)
  {
    ppn_t key = it->first;
    IPTEntry* ipt_entry = it->second;
    ArchMemory* entry_arch = ipt_entry->archmem_;
    assert(entry_arch && "No archmem in IPTEntry");

    // this locking will not solve deadlock completely, but this is debug func so who cares
    int locked_by_us = 0;
    if (!entry_arch->archmemory_lock_.isHeldBy((Thread*) currentThread))
    {
      entry_arch->archmemory_lock_.acquire();
      locked_by_us = 1;
    }
    
    ppn_t disk_offset = (ppn_t) entry_arch->getDiskLocation(ipt_entry->vpn_);
    assert(key == disk_offset && "checkRampMapConsistency: ppn in disk_map_ (key) does not match disk offset in ArchMemory\n");
    if (locked_by_us)
    {
      entry_arch->archmemory_lock_.release();
    }
  }
}

void IPTManager::checkSwapMetaDataConsistency()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::checkSwapMetaDataConsistency called but IPT not locked\n");

  ustl::set<ppn_t> uniqueKeys;
  for (auto entry : ram_map_)
  {
    uniqueKeys.insert(entry.first);
  }

  assert(uniqueKeys.size() == swap_meta_data_.size() && "checkSwapMetaDataConsistency: swap_meta_data_ and ram_map_ are not in sync\n");
  
  for (auto key : uniqueKeys)
  {
    if (swap_meta_data_.find(key) == swap_meta_data_.end())
    {
      debug(SWAPPING, "IPTManager::checkSwapMetaDataConsistency: Key %zu in ram_map_ does not exist in swap_meta_data_\n", key);
      assert(0 && "checkSwapMetaDataConsistency: swap_meta_data_ and ram_map_ are not in sync\n");
    }
  }
}

bool IPTManager::isThereAnyPageToSwapOut()
{
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::isThereAnyPageToSwapOut called but IPT not locked\n");

  if (pra_type_ == PRA_TYPE::RANDOM)
  {
    ustl::vector<ppn_t> unique_keys = getUniqueKeysInRamMap();

    if (!unique_keys.empty()) {
      return true;
    }
  }
  else if (pra_type_ == PRA_TYPE::NFU)
  {
    uint32 min_counter = UINT32_MAX;

    for(auto& pair : swap_meta_data_)
    {
      uint32 counter = pair.second;
      if (counter < min_counter)
      {
        min_counter = counter;
      }
    }

    if (min_counter < UINT32_MAX) {
      return true;
    }
  }

  return false;
}