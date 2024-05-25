#include "IPTManager.h"
#include "debug.h"
#include "assert.h"
#include "debug.h"
#include "Thread.h"
// #include "ArchMemory.h"

////////////////////// IPTEntry //////////////////////

IPTEntry::IPTEntry(ppn_t ppn, size_t vpn, ArchMemory* archmem) : ppn(ppn), vpn(vpn), archmem(archmem) {}

bool IPTEntry::isLocked() {
  return archmem->archmemory_lock_.isHeldBy((Thread*) currentThread);
}


////////////////////// IPTManager //////////////////////
IPTManager* IPTManager::instance_ = nullptr;

IPTManager::IPTManager() : IPT_lock_("IPTManager::IPT_lock_") {
  assert(!instance_);
  instance_ = this;
}

IPTManager* IPTManager::instance()
{
  return instance_;
}

IPTEntry* IPTManager::lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::lookupEntryInRAM: Looking up entry in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
  auto range = ram_map_.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->vpn == vpn && it->second->archmem == archmem) {
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

  IPTEntry* entry = new IPTEntry(ppn, vpn, archmem);
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
    if (it->second->vpn == vpn && it->second->archmem == archmem) {
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
    debug(SWAPPING, "IPTManager::moveEntry: At offset %zu in the destination map %s exists already a value.\n", ppn_destination, destination_as_string);
    assert(0 && "Offset in destination map not free\n");
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
    for (auto it = range.first; it < range.second; it++) 
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



template<typename... Args>
void IPTManager::lockArchmemInOrder(Args... args)
{
    ustl::vector<Mutex*> vec = { args... };
    ustl::sort(vec.begin(), vec.end());

    // Lock the mutexes in order
    for(auto& m : vec) {
      assert(m && "Mutex for archmem is null");
      m->acquire();
    }
}


