#include "IPTManager.h"
#include "debug.h"
#include "assert.h"
#include "debug.h"
#include "Thread.h"
#include "Syscall.h"
#include "uset.h"
#include "SwappingManager.h"
#include "ArchMemory.h"

#define INVALID_PPN -1

////////////////////// IPTEntry //////////////////////

IPTEntry::IPTEntry(size_t vpn, ArchMemory* archmem) : vpn_(vpn), archmem_(archmem) {}

bool IPTEntry::isLocked() {
  return archmem_->archmemory_lock_.isHeldBy((Thread*) currentThread);
}


////////////////////// IPTManager //////////////////////
IPTManager* IPTManager::instance_ = nullptr;

IPTManager::IPTManager() : IPT_lock_("IPTManager::IPT_lock_") {
  assert(!instance_);
  instance_ = this;
  pra_type_ = PRA_TYPE::AGING;
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
void debugRandomGenerator()
{
  ustl::vector<size_t> randomNumbers;
  for (int i = 0; i < 1024; ++i) {
    size_t randomNumber = randomNumGenerator();
    randomNumbers.push_back(randomNumber);
  }
  for (int i = 0; i < 1024; i++)
  {
    debug(MINH, "random num is %zu\n", randomNumbers[i] % 1024);
  }
  assert(0);
}

ustl::vector<ppn_t> IPTManager::getUniqueKeysInIPT()
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

  if (pra_type_ == PRA_TYPE::RANDOM)
  {
    debug(SWAPPING, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA RANDOM\n");
    // debugRandomGenerator();    // for debugging purposes

    size_t random_num = randomNumGenerator();
    debug(MINH, "IPTManager::findPageToSwapOut: random num : %zu\n", random_num);

    ustl::vector<ppn_t> unique_keys = getUniqueKeysInIPT();
    size_t random_ipt_index = random_num % unique_keys.size();

    ppn_retval = (size_t) (unique_keys[random_ipt_index]);
  }
  else if (pra_type_ == PRA_TYPE::AGING)
  {
    debug(SWAPPING, "IPTManager::findPageToSwapOut: Finding page to swap out using PRA AGING\n");

    size_t random_num = randomNumGenerator();
    debug(MINH, "IPTManager::findPageToSwapOut: random num : %zu\n", random_num);

    ustl::vector<ppn_t> unique_keys = getUniqueKeysInIPT();
    size_t random_ipt_index = random_num % unique_keys.size();

    ppn_retval = (size_t) (unique_keys[random_ipt_index]);
  }

  assert(ppn_retval != INVALID_PPN && "IPTManager::findPageToSwapOut: failed to find a valid ppn\n");
  debug(SWAPPING, "IPTManager::findPageToSwapOut: Found page to swap out: ppn=%d\n", ppn_retval);
  return ppn_retval;
}


IPTEntry* IPTManager::lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::lookupEntryInRAM: Looking up entry in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
  auto range = ram_map_.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->vpn_ == vpn && it->second->archmem_ == archmem) {
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

  // checking is swap_meta_data_ is in sync with the ram_map_. This is not necessary and slow down the system, but it is good for debugging
  checkSwapMetaDataConsistency();

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

  // checking is swap_meta_data_ is in sync with the ram_map_. This is not necessary and slow down the system, but it is good for debugging
  checkSwapMetaDataConsistency();

  debug(IPT, "IPTManager::removeEntryIPT: Entry found in map %s, seems valid. Removing\n", (map_type == IPTMapType::RAM_MAP ? "RAM_MAP" : "DISK_MAP"));

  // Actually remove the pte from the entry
  auto range = map->equal_range(ppn);
  for (auto it = range.first; it != range.second;)
  {
    if (it->second->vpn_ == vpn && it->second->archmem_ == archmem)
    {
      debug(IPT, "IPTManager::removeEntryFromRAM: Entry found and removed from RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
      delete it->second;    // delete the pointer created using "new"
      it = map->erase(it);  // erase the value, the key still exists if theres other values mapped to it
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
}

ustl::vector<IPTEntry*> IPTManager::moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
  const char* source_as_string = (source == IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");
  const char* destination_as_string = (source != IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");

  debug(SWAPPING, "IPTManager::moveEntry: moving entry at offset %zu in %s to offset %zu in %s\n", ppn_source, source_as_string, ppn_destination, destination_as_string);
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::moveEntry called but IPT not locked\n");

  auto* source_map = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  auto* destination_map = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);

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

    entry_arch->archmemory_lock_.acquire();
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

    entry_arch->archmemory_lock_.release();
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

    entry_arch->archmemory_lock_.acquire();
    ppn_t disk_offset = (ppn_t) entry_arch->getDiskLocation(ipt_entry->vpn_);
    assert(key == disk_offset && "checkRampMapConsistency: ppn in disk_map_ (key) does not match disk offset in ArchMemory\n");
    entry_arch->archmemory_lock_.release();
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