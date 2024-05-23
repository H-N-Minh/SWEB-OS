#include "IPTManager.h"
#include "debug.h"
#include "assert.h"
#include "debug.h"
#include "Thread.h"

////////////////////// IPTEntry //////////////////////

IPTEntry::IPTEntry(ppn_t ppn, size_t vpn, ArchMemory* archmem) : ppn(ppn), vpn(vpn), archmem(archmem) {}

bool IPTEntry::isLocked() {
  return archmem->archmemory_lock_.isHeldBy((Thread*) currentThread);
}


////////////////////// IPTManager //////////////////////

IPTManager::IPTManager() : IPT_lock_("IPTManager::IPT_lock_") {}

ustl::shared_ptr<IPTEntry> IPTManager::lookupEntryInRAM(ppn_t ppn, size_t vpn, ArchMemory* archmem) {
  debug(IPT, "IPTManager::lookupEntryInRAM: Looking up entry in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
  auto range = ram_map_.equal_range(ppn);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second->vpn == vpn && it->second->archmem == archmem) {
      debug(IPT, "IPTManager::lookupEntryInRAM: Entry found in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
      return it->second;
    }
  }
  debug(IPT, "IPTManager::lookupEntryInRAM: Entry not found in RAM: ppn=%zu, vpn=%zu\n", ppn, vpn);
  return ustl::shared_ptr<IPTEntry>();
}

ustl::shared_ptr<IPTEntry> IPTManager::lookupEntryInDisk(diskoffset_t diskOffset) {
  debug(IPT, "IPTManager::lookupEntryInDisk: Looking up entry in Disk: diskOffset=%zu\n", diskOffset);
  auto it = disk_map_.find(diskOffset);
  if (it != disk_map_.end()) {
    debug(IPT, "IPTManager::lookupEntryInDisk: Entry found in Disk: diskOffset=%zu\n", diskOffset);
    return it->second;
  }
  debug(IPT, "IPTManager::lookupEntryInDisk: Entry not found in Disk: diskOffset=%zu\n", diskOffset);
  return ustl::shared_ptr<IPTEntry>();
}

void IPTManager::insertEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  debug(SWAPPING, "IPTManager::insertEntryIPT: inserting ppn: %zu, vpn: %zu, archmem: %zu\n", ppn, vpn, (size_t) archmem);
  assert(map_type == IPTMapType::NONE && archmem && "IPTManager::insertEntryIPT called with a nullptr argument\n");
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && archmem->archmemory_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::insertEntryIPT called without fully locking\n");

  auto* map = (map_type == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  // TODO: check if the entry already exists in the map. If it does, assert fail
  ustl::shared_ptr<IPTEntry> entry = ustl::make_shared<IPTEntry>(ppn, vpn, archmem);
  map->insert({ppn, entry});

  debug(SWAPPING, "IPTManager::insertIPT: successfully inserted to IPT\n");
}

void IPTManager::removeEntryIPT(IPTMapType map_type, size_t ppn, size_t vpn, ArchMemory* archmem)
{
  // Error checking
  debug(SWAPPING, "IPTManager::removeEntryIPT: removing ppn: %zx, archmem: %zx\n", ppn, (size_t) archmem);
  assert(map_type == IPTMapType::NONE && archmem && "IPTManager::removeEntryIPT called with a nullptr argument\n");
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
      it = map->erase(it);
    } else {
      ++it;
    }
  }

  debug(SWAPPING, "IPTManager::removeIPT: successfully removed from IPT\n");
}

void IPTManager::moveEntry(IPTMapType source, size_t ppn_source, size_t ppn_destination)
{
  debug(SWAPPING, "IPTManager::moveEntry: moving entry ppn %zu to ppn %zu\n", ppn_source, ppn_destination);
  assert(IPT_lock_.isHeldBy((Thread*) currentThread) && "IPTManager::moveEntry called but IPT not locked\n");
  assert(source == IPTMapType::NONE && "IPTManager::moveEntry invalid parameter\n");
  // TODO: assert that all archmem of the entry are locked

  auto* source_map = (source == IPTMapType::RAM_MAP ? &ram_map_ : &disk_map_);
  auto* destination_map = (source == IPTMapType::RAM_MAP ? &disk_map_ : &ram_map_);

  // Check if the entry already exists in the destination map
  if (destination_map->find(ppn_destination) != destination_map->end())
  {
    debug(SWAPPING, "IPTManager::moveEntry ppn %zu already exists in the destination map\n", ppn_destination);
    assert(0 && "IPTManager::moveEntry: ppn_destination already exists in destination map\n");
  }

  // Check if the entry exists in the source map
  if (source_map->find(ppn_source) == source_map->end())
  {
    debug(SWAPPING, "IPTManager::moveEntry ppn %zu not found in the source map\n", ppn_source);
    assert(0 && "IPTManager::moveEntry: The given ppn doesnt exist in the specifed map\n");
  }
  else
  {
    // Move all values with the corresponding key from source_map to destination_map
    auto range = source_map->equal_range(ppn_source);
    for (auto it = range.first; it != range.second; ++it) {
      destination_map->insert({ppn_destination, it->second});
    }

    // Remove the entry from the source map
    source_map->erase(ppn_source);
  }
}

IPTMapType IPTManager::isInWhichMap(uint64 ppn)
{
  if (ram_map_.find(ppn) != ram_map_.end())
    return IPTMapType::RAM_MAP;
  if (disk_map_.find(ppn) != disk_map_.end())
    return IPTMapType::DISK_MAP;
  return IPTMapType::NONE;
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