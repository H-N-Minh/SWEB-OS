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
  debug(SWAPPING, "IPTManager::insertEntryIPT: inserting ppn: %zu, vpn: %zu, archmem: %p\n", ppn, vpn, archmem);
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::insertEntryIPT called without fully locking\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  // TODO: check if the entry already exists in the map. If it does, assert fail
  if(!KeyisInMap(ppn, map_type))
  {
    if(map_type == IPTMapType::RAM_MAP)
    {
      pages_in_ram_++;
    }
    else
    {
      pages_on_disk_++;
    }
  }

  IPTEntry* entry = new IPTEntry(vpn, archmem);
  map->insert({ppn, entry});


  debug(SWAPPING, "IPTManager::insertIPT: successfully inserted to IPT\n");
}

void IPTManager::removeEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  // Error checking
  debug(SWAPPING, "IPTManager::removeEntryIPT: removing ppn: %zx, archmem: %p\n", ppn, archmem);
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::removeEntryIPT called but not fully locked\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  
  if (map->find(ppn) == map->end())
  {
    debug(SWAPPING, "IPTManager::removeEntryIPT ppn %zu not found in the specified map\n", ppn);
    assert(0 && "IPTManager::removeEntryIPT: ppn doesnt exist in IPT\n");
  }

  // Actually remove the pte from the entry
  // TODO: check if the IPTEntry*, that is being removed, exists in the map. If it doesnt, assert fail
  auto range = map->equal_range(ppn);
  for (auto it = range.first; it != range.second;) {
    if (it->second->vpn_ == vpn && it->second->archmem_ == archmem) {
      debug(IPT, "IPTManager::removeEntryFromRAM: Entry found and removed from RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
      delete it->second;
      it = map->erase(it);
      break;
    } else {
      ++it;
    }
  }

  if(!KeyisInMap(ppn, map_type))
  {
    if(map_type == IPTMapType::RAM_MAP)
    {
      pages_in_ram_--;
    }
    else
    {
      pages_on_disk_--;
    }
  }


  debug(SWAPPING, "IPTManager::removeIPT: successfully removed from IPT\n");
}

ustl::vector<IPTEntry*> IPTManager::moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
  const char* source_as_string = (source == IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");
  const char* destination_as_string = (source != IPTMapType::RAM_MAP ? "RAM-MAP" : "DISK-MAP");

  debug(SWAPPING, "IPTManager::moveEntry: moving entry at offset %zu in %s to offset %zu in %s\n", ppn_source, source_as_string, ppn_destination, destination_as_string);
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::moveEntry called but IPT not locked\n");
  // TODO: assert that all archmem of the entry are locked

  auto* source_map = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  auto* destination_map = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);

  ustl::vector<IPTEntry*> ipt_entries;
  // Check if the entry already exists in the destination map
  if (destination_map->find(ppn_destination) != destination_map->end())
  {
    // Compare the values of source_map and destination_map to see if its the same entry
    auto source_it = source_map->find(ppn_source);
    auto destination_it = destination_map->find(ppn_destination);
    if (source_it != source_map->end() && destination_it != destination_map->end())
    {
      if (source_it->second == destination_it->second)
      {
        debug(SWAPPING, "IPTManager::moveEntry ppn %zu already exists in the destination map\n", ppn_destination);
        assert(0 && "IPTManager::moveEntry: ppn_destination already exists in destination map\n");
      }
    }
  }
  // Check if the entry exists in the source map
  else if (source_map->find(ppn_source) == source_map->end())
  {
    debug(SWAPPING, "IPTManager::moveEntry: At offset %zu in the source map %s is no value value.\n", ppn_source, source_as_string);
    assert(0 && "No value at offset in source map\n");
  }
  else
  {
    // Move all values with the corresponding key from source_map to destination_map
    auto range = source_map->equal_range(ppn_source);
    for (auto it = range.first; it != range.second; ++it)
    {
      destination_map->insert({ppn_destination, it->second});
      ipt_entries.push_back(it->second);
    }

    // Remove the entry from the source map
    source_map->erase(ppn_source);

  if(source == IPTMapType::DISK_MAP)
  {
    pages_in_ram_++;
    pages_on_disk_--;
  }
  else
  {
    pages_on_disk_++;
    pages_in_ram_--;
  }
  }

  return ipt_entries;
}

 bool IPTManager::KeyisInMap(size_t offset, IPTMapType maptype)
 {
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
  IPT_lock_.acquire();

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

  IPT_lock_.release();
}

void IPTManager::checkDiskMapConsistency()
{
  IPT_lock_.acquire();

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

  IPT_lock_.release();
}
